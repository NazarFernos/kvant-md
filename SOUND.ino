dma_tube_config  dma_cfg2;
const int sound_test[]={120,160,220,400,600,800,1000};
const float Log_Tab[]= {0.09, 0.125, 0.18, 0.25, 0.35, 0.5, 0.7, 1.0};

const int8_t Piano[]  = 
{             
0,0,0,0,0,1,2,3,4,6,7,9,13,15,18,20,23,26,31,36,40,43,46,51,
54,56,57,57,57,56,54,50,46,42,38,33,28,23,19,16,14,11,8,5,3,
0,-1,-1,-1,1,3,5,7,11,15,20,23,26,28,35,37,41,45,50,54,57,59,
64,69,71,74,77,82,86,89,93,96,99,103,106,108,110,112,114,115,
116,118,119,120,121,123,124,125,126,126,126,126,126,126,126,
126,126,126,126,125,124,123,121,120,119,118,116,115,113,112,
110,108,105,102,98,95,90,86,81,74,71,68,63,60,54,50,43,38,33,
28,23,15,10,7,3,0,-4,-7,-11,-15,-18,-22,-26,-30,-33,-36,-40,
-43,-47,-49,-51,-54,-57,-60,-62,-64,-66,-66,-66,-66,-66,-64,
-62,-59,-55,-52,-49,-48,-44,-42,-39,-37,-33,-31,-31,-31,-33,
-36,-38,-41,-46,-50,-55,-60,-64,-69,-74,-80,-87,-95,-101,-104,
-108,-112,-114,-116,-117,-118,-119,-120,-120,-120,-120,-120,
-119,-117,-116,-114,-113,-110,-107,-104,-101,-97,-94,-90,-88,
-86,-86,-87,-91,-93,-96,-96,-94,-92,-88,-84,-80,-74,-72,-67,
-62,-56,-51,-46,-44,-39,-33,-28,-22,-18,-15,-11,-9,-5,-3,-1,0,0,0
};
 
#define F_Sound_divider F_CPU /(SAMPLES_Sound-1)
#define SAMPLES_Sound  sizeof(Piano)
volatile int Sound_val[SAMPLES_Sound]; 
volatile int val_s[SAMPLES_Sound];
int Sound_tic=-1;
float amp;
int zero;
volatile int Prec;
volatile bool s_flag=false;

//-------------------------------------------------Sound_conf()------------------------------------------------------------------------------------

HardwareTimer Timer_3(3);
timer_dev *dev2 = PIN_MAP[Out_Sound].timer_device;//Timer3
void Sound_conf()
 {
  uint8 cc_channel2 = PIN_MAP[Out_Sound].timer_channel;//Channel4
  
  
  Timer_3.pause();
  timer_dma_set_base_addr(dev2, TIMER_DMA_BASE_CCR4);
  timer_dma_set_burst_len(dev2, 1);
  timer_dma_enable_req(dev2, cc_channel2);
  timer_set_reload(dev2,282 );// 1000 Гц
  timer_set_prescaler(dev2, 0);
  
  //T3C4 DMA1_CH3
    dma_init(DMA1);
    dma_cfg2.tube_dst = &(dev2->regs.gen->DMAR);
    dma_cfg2.tube_dst_size = DMA_SIZE_32BITS;
    dma_cfg2.tube_src =  Sound_val;
    dma_cfg2.tube_src_size = DMA_SIZE_32BITS;   
    dma_cfg2.tube_nr_xfers = SAMPLES_Sound;
    dma_cfg2.tube_flags = DMA_CFG_SRC_INC | DMA_CFG_CIRC | DMA_CFG_CMPLT_IE;  
    dma_cfg2.tube_req_src = DMA_REQ_SRC_TIM3_CH4;
    dma_cfg2.target_data = 0;  
    dma_tube_cfg(DMA1, DMA_CH3, &dma_cfg2);
    dma_attach_interrupt(DMA1, DMA_CH3, s_fun);
    dma_enable(DMA1, DMA_CH3);
    snd_OFF(); 
    Timer_3.resume();
 }

void s_fun()
{
   if(s_flag)
    {
    timer_set_reload(dev2,Prec );
    for(int i=0;i<SAMPLES_Sound;i++)Sound_val[i]=val_s[i]; 
    s_flag=false;    
    }
   
}

//---------------------------------------------VDI_TONE------------------------------------------------------

int VDI_TONE(int s_vdi, float g )
{
 
 if((Mem.Sound_schem==2||Mem.Sound_schem==3)&&(Mem.Filter_select==1||Mem.Filter_select==2||Mem.Filter_select==3))
  {   
   int vdi_tone=0;
   if (s_vdi>=0)vdi_tone= map(s_vdi, 0,90, 280,1200);//Звук - Цвет_мет 
   if (s_vdi< 0)vdi_tone= map(s_vdi, -90,-1, 60,220);//Звук - Железо                                          
   if(Mem.Sound_schem==2)Sound_tic=20;// Фиксированная длительность 
   if(Mem.Sound_schem==3)Sound_tic=14+round(g*2.2);// Длительность звука от уровня сигнала
   amp = Max_amp;
   return vdi_tone;
  }
 
 if(Mem.Sound_schem==0||Mem.Sound_schem==1||Mem.Filter_select==0||Mem.Filter_select==4)
  {
   float Lev_S;
   if (Mem.Sound_schem==1&&g<=6.0)Lev_S=g*0.16666; 
   else Lev_S=Max_amp ;// Максимальная громкость                                       
   if (s_vdi>=0) Go_Sound(map(s_vdi, 0,90, 280,1200),Lev_S);
   if (s_vdi< 0) Go_Sound(map(s_vdi, -90,-1, 60,220),Lev_S);  
  }
  millis_1 = millis();
}

//------------------------------------snd_OFF()--------------------------------------------------------------
void snd_OFF()
{ 
for(int i=0;i<SAMPLES_Sound;i++)Sound_val[i]=zero; 
}
//-------------------------------------Go_Sound()-------------------------------------------------------------------------
void Go_Sound(int Sound_frequency, float level )
{
 Prec = F_Sound_divider/Sound_frequency;
 int lev =round( ((Prec-1)*Log_Tab[Mem.vol_Sound])*level);
 zero = (Prec-1)/2;

 if( Mem.vol_Sound<0)lev=0;
 
 for(int i=0;i<SAMPLES_Sound;i++)val_s[i]=zero+map(int(Piano[i]),-127,127,(lev/2)*-1,lev/2);
 s_flag=true;         
 
}
//------------------------------------ Soft_Sound----------------------------------------------
void Fading_Sound(int F_s)//F_s частота звука
{
 
if(Sound_tic<0)snd_OFF();
if(Sound_tic>14)Go_Sound(F_s, amp); 
if(Sound_tic<=14&&Sound_tic>=0) {amp*=0.86f; Go_Sound(F_s, amp );}
Sound_tic--;
}


//------------------------------------------------error_Sound-------------------------------------------------------------------------------------------

void error_Sound()
{ 
 static bool svitch;
 if (svitch) {Go_Sound( 400, Max_amp);} 
 else {Go_Sound(800, Max_amp);} 
 svitch=!svitch; 
 millis_1 = millis();
}

//-----------------------------------------BEEP()-------------------------------------------------------------------------------------------
void BEEP(int f) 
{
 Go_Sound( f, Max_amp); 
 delay(100); 
 snd_OFF();
}

//------------------------------------------sound_level()----------------------------------------------------------------------------------
void sound_level()
{
 lcd.clear();
while (true)
  {  
   lcd.setCursor(0,0);
   if(Mem.Sound_schem==0) lcd.print("Full Volume    "); 
   if(Mem.Sound_schem==1) lcd.print("Adaptive Volume"); 
   if(Mem.Sound_schem==2) lcd.print("Fading Sound   "); 
   if(Mem.Sound_schem==3) lcd.print("Fading Sound+  "); 
      
   lcd.setCursor(0,1); lcd.write(2);
   for (int i=0; i<= Mem.vol_Sound; i++){lcd.print(char(255));} 
   for(int i= Mem.vol_Sound; i<7; i++) lcd.print(char(219));
   
   if( check_button()==false){Button_FLAG=false;}
   //if( Button_read(0)) {Button_enter();}
   if( Button_read(2)) {Button_enter();  Mem.vol_Sound--; }
   if( Button_read(3)) {Button_enter(); Mem.Sound_schem++; if(Mem.Sound_schem>3)Mem.Sound_schem=0;}
   if( Button_read(4)) {Button_enter();  Mem.vol_Sound++; } 
   if( Button_read(5)) {Button_enter(); break;}
    Mem.vol_Sound=constrain( Mem.vol_Sound,-1 ,7 ); 
  }  

lcd.clear();
} 


//-------------------------------------------test_sound--------------------------------------------------------------------------------------------------------
void test_sound(int mode)
{
if(mode==1)for(int i=0;i<(sizeof(sound_test)/4);i++) BEEP(sound_test[i]);
if(mode==2)for(int i=(sizeof(sound_test)/4)-1;i>=0;i--) BEEP(sound_test[i]);
}
//------------------------------------------Sound_Col-----------------------------------------------------------------
