
#include <libmaple/dma.h>
#include <libmaple/adc.h>
#include <libmaple/pwr.h>
#include <libmaple/scb.h>
#include <Wire.h>
#define DAC 0x60 //Адрес MCP4725, или 0x61 0x62 0x63
#define EEPROM 0x50 //Адрес внешнего EEPROM

#include "LiquidCrystalM.h"
LiquidCrystal lcd(PB9,PA12,PB5,PB4,PB3,PA15);

#define MUTE PB11
#define LED_LCD PA11
#define Out_Sound  PB1 // Вывод звука

#define CLOCK_pin  PA5
#define DATA_pin_1 PA6
#define DATA_pin_2 PA7

#define current_cal 931 //делителем калибруем показания тока ТХ
#define Ferrit_VDI -90

#define num_prof 4
struct strMem    //Структура данных для EEPROM
{                  
 int16_t amp_TX[num_prof];// Амплитуда TX 0...1600
 int16_t F_TX[num_prof];  //Частота TX = F_CPU/2/Prescaler*SAMPLES
 int8_t Comp_Phase[num_prof];      //Компенсация начальной фазы 0...35 / шаг 5 градусов .
 int8_t Turn_phase[num_prof];      //переворот фазы 1 или -1              
 int16_t Adjustment_Phase[num_prof]; // Корекция фазы                 
 int8_t Cut_VDI_H[num_prof];// Граница VDI цвет
 int8_t Cut_VDI_L[num_prof];// Граница VDI чёрный
 byte Prof;               //Профиль1-2-3-4
 byte Threshold;       // Порог отсечки сигналов 
 byte Sound_schem; // Звуковая схема
 int8_t vol_Sound;   // Громкость звука
 byte Filter_select; // Фильтр F.N.S.M.m  
 bool LED_LCD;
 byte Mask; // Mask 1 Parabola 
}Mem;  

const int button_input[]={PA8,PB15,PB14,PB13,PB12};
bool Button_FLAG=false;
int Otsechka1Sempla =true;
int Sound_block = 30;

byte Speaker[]={0b00011,0b00101,0b11001,0b11001,0b11001,0b11001,0b00101,0b00011};
byte Battery[]={0b01110,0b11111,0b10001,0b10001,0b10001,0b10001,0b10001,0b11111};

const float Max_amp =1.0F;
int Sound_tone=400;
int vdi_mem;

unsigned int micros_1,micros_2;
unsigned int millis_1,millis_2;
const int ADC_max=  8388600;
const int ADC_min= -8388600;

adc_reg_map *regs;
#define Ared_samp 512
volatile unsigned int *Sin_mas;
volatile int Ared_count;

int Xa=0,Ya=0;
bool Error=false;
#define SAMPLES_TX 36
HardwareTimer Timer_2(2);
void TX_conf()
 {
  Timer_2.pause();
  Timer_2.setCount(0);
  
  Timer_2.setPrescaleFactor(Mem.F_TX[Mem.Prof]);
  Timer_2.setOverflow(SAMPLES_TX-1);
 
  timer_oc_set_mode(TIMER2, 2, TIMER_OC_MODE_INACTIVE_ON_MATCH, TIMER_OC_PE);
  timer_oc_set_mode(TIMER2, 3, TIMER_OC_MODE_INACTIVE_ON_MATCH, TIMER_OC_PE);
  timer_oc_set_mode(TIMER2, 4, TIMER_OC_MODE_INACTIVE_ON_MATCH, TIMER_OC_PE);     
  Timer_2.setCompare(TIMER_CH2,0);
  Timer_2.setCompare(TIMER_CH3,0);
  Timer_2.setCompare(TIMER_CH4,0);
  
  Timer_2.refresh();
  Timer_2.resume();
  
  delay (2);
  
  Timer_2.pause();
  Timer_2.setCount(0); 
  
  timer_oc_set_mode(TIMER2, 2, TIMER_OC_MODE_TOGGLE, TIMER_OC_PE); 
  timer_oc_set_mode(TIMER2, 3, TIMER_OC_MODE_TOGGLE, TIMER_OC_PE); 
  timer_oc_set_mode(TIMER2, 4, TIMER_OC_MODE_TOGGLE, TIMER_OC_PE);
  Timer_2.setCompare(TIMER_CH2,Mem.Comp_Phase[Mem.Prof]);
  Timer_2.setCompare(TIMER_CH3,1);
  Timer_2.setCompare(TIMER_CH4,19);                                        
  
  Timer_2.refresh();
  Timer_2.resume();    
 }
 
void Serial_data(int32_t A,int32_t B )
{
  Serial.print('A'); 
  Serial.print(A); // Xa Красный
  
  Serial.print('B'); 
  Serial.print(B);// Ya Зелёный 
  
  Serial.print('#');// 
}
//======================================================setup()==============================================================
void setup() 
{ 
 adc_set_sample_rate(ADC1, ADC_SMPR_1_5);
 afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY); // release PB3 and PB5

 Wire.begin();
 //Serial.end();
 Serial.begin(115200);
  
 for(int i=0; i<5; i++) pinMode(button_input[i],INPUT_PULLUP); // buttons input setup  
 pinMode(PA1, PWM); //TX 
 pinMode(PA2, PWM); //Yy 
 pinMode(PA3, PWM); //Xy
 
 pinMode(DATA_pin_1,INPUT); 
 pinMode(DATA_pin_2, INPUT); 
 pinMode(CLOCK_pin, OUTPUT);  
 
 lcd.begin(16, 2);
 lcd.print("Check EEPROM");
 
 if(init_eeprom_mem()==true) EEPROM_read(1, Mem, EEPROM);// Если нет ошибки - чтение из EEPROM 
 set_current_TX(Mem.amp_TX[Mem.Prof]);  
 
 pinMode(LED_LCD,OUTPUT); digitalWrite(LED_LCD,Mem.LED_LCD);
 pinMode(MUTE,OUTPUT_OPEN_DRAIN); digitalWrite(MUTE,LOW);//MUTE OFF

 Sound_conf();
 pinMode(Out_Sound, PWM);
 
 lcd.clear();
 lcd.print("KVANT_HX");
 
 test_sound(1);
 delay(2000); 
  
 TX_conf();
 lcd.createChar(2,Speaker);
 lcd.createChar(1,Battery);

 LCD_start();  
}

//=====================================================loop()============================================================
void loop() 
{
static int Tic=0;
static int Max_M=0;
static int Cut_Semp=0;

ADC_Read_Data(false);

int button_check = check_button();
if (button_check>0&&Button_FLAG==false){Button_CLK(button_check);}

int VDI =round(degrees(atan2(Xa,Ya)))+Mem.Adjustment_Phase[Mem.Prof];
 if(VDI>180)VDI-=360;
 if(VDI<-180)VDI+=360;
int Magnitude = max(abs(Xa),abs(Ya))+3*min(abs(Xa),abs(Ya))/8;
Magnitude = Magnitude_mask(Magnitude,VDI);
  
if( VDI<=Mem.Cut_VDI_H[Mem.Prof]&&VDI>Mem.Cut_VDI_L[Mem.Prof]&&Magnitude>Mem.Threshold*16&&Error==false)
    {                  
    if(Magnitude>=Max_M&&Sound_block<=0)
      {
       Max_M=Magnitude; 
       float M =log(float(Magnitude)/64);
       if(M>7.0){M=7.0;} 
       if(M<1.0){M=1.0;}  
       Magnitude=round(M);               
       if (Cut_Semp>0&&Otsechka1Sempla==true&&Error==false)Sound_tone = VDI_TONE(VDI,M);
       if (Otsechka1Sempla==false&&Error==false)Sound_tone = VDI_TONE(VDI,M);
       lcd.setCursor(0,1);
       for(int i=0; i<Magnitude; i++)lcd.write(char(255)); for(int i=Magnitude; i<7; i++)lcd.write('<'); 
       lcd.setCursor(3,0);if(VDI>=0)lcd.print("+"); lcd.print(VDI); lcd.print(" ");//Индикация VDI
       Cut_Semp++;
       vdi_mem=VDI;
      }        
    }
 else  {Max_M=0;Cut_Semp=0;if((Mem.Filter_select==4||Mem.Filter_select==0)&&Error==false)snd_OFF();} 
 V_bat_ind();   
 
millis_2 = millis()- millis_1;
if((Mem.Sound_schem==0||Mem.Sound_schem==1)&&(Mem.Filter_select==1||Mem.Filter_select==2||Mem.Filter_select==3)) if(millis_2>100)snd_OFF();   
if((Mem.Sound_schem==2||Mem.Sound_schem==3)&&(Mem.Filter_select==1||Mem.Filter_select==2||Mem.Filter_select==3)&&Error==false) Fading_Sound(Sound_tone);
if(Sound_block>0)Sound_block--;
}

//---------------------------------------------ADC_value_processing()---------------------------------------------------------------------------
int ADC_Read_Data(bool svitch)
{      
 static int Old_X,Old_Y;
  
 int  adc_data_1=0; 
 int  adc_data_2=0;
  
   while (digitalRead(DATA_pin_1) || digitalRead(DATA_pin_2));
   
            for (uint8_t i = 0; i < 24; i++) 
              {
                digitalWrite(CLOCK_pin, HIGH);
                delayMicroseconds(1);
                
                adc_data_1 <<= 1;
                if (digitalRead(DATA_pin_1)) adc_data_1 |= 1;
                
                adc_data_2 <<= 1;
                if (digitalRead(DATA_pin_2)) adc_data_2 |= 1;
                
                digitalWrite(CLOCK_pin, LOW);
                delayMicroseconds(1);               
                
              }          
                
              for (uint8_t i = 0; i < 2; i++) 
                {
                 digitalWrite(CLOCK_pin, HIGH);
                 delayMicroseconds(1);
                 digitalWrite(CLOCK_pin, LOW);
                 delayMicroseconds(1);
                }     
                        
        
        if (adc_data_1 & 0x800000) adc_data_1 |= 0xFF000000;  // отрицательные
        if (adc_data_2 & 0x800000) adc_data_2 |= 0xFF000000;  // отрицательные 
 

  if(adc_data_1>ADC_max||adc_data_1<ADC_min||adc_data_2>ADC_max||adc_data_2<ADC_min){ error_Sound(); Error=true;}
  else Error=false;
    
  adc_data_1*=Mem.Turn_phase[Mem.Prof];
  adc_data_2*=Mem.Turn_phase[Mem.Prof];

   Xa=iir_AL(adc_data_1);//ФНЧ-X
   Ya=iir_BL(adc_data_2);//ФНЧ-Y 
   
 if(svitch) return max(abs(Xa),abs(Ya))+3*min(abs(Xa),abs(Ya))/8;

 if(Mem.Filter_select==0||Mem.Filter_select==4)// M,U 
  {    
    Xa=iir1A_M(Xa);
    Ya=iir1B_M(Ya);                    
  }
 else //S, N, F,
  {
    Xa=iir2A(Xa);
    Ya=iir2B(Ya);
  }

    Xa=(Xa+Old_X)/2; Old_X=Xa; // ФНЧ 
    Ya=(Ya+Old_Y)/2; Old_Y=Ya; // ФНЧ  

    Serial_data(Xa, Ya);
}

//-------------------------------------------------ADC_read-----------------------------------------------------------------

int ADC_read(int input)
{ 
 Ared_count=0;//Счётчик семплов 
 Sin_mas = new volatile unsigned int[Ared_samp]; 

    adc_dev *dev = PIN_MAP[input].adc_device;
    regs = dev->regs;
    adc_set_reg_seqlen(dev, 1);
    regs->SQR3 = PIN_MAP[input].adc_channel;
 
 Timer_2.attachCompare3Interrupt(Timer2_irq1);// Старт 
 Timer_2.attachCompare4Interrupt(Timer2_irq2);// 

 while (Ared_count!=Ared_samp);//Ждём окончания накопления семплов
 Timer_2.detachCompare3Interrupt();// Стоп 
 Timer_2.detachCompare4Interrupt();//      
   int X_Y[4];// массив данных АЦП для XY
   for(int i=0;i<4;i++)X_Y[i]=0;//Отчистка массива значений ADC
   for(int i=0;i<Ared_samp;i=i+4)
    {
     X_Y[0] += Sin_mas[i];
     X_Y[1] += Sin_mas[i+2];  
     X_Y[2] += Sin_mas[i+1];
     X_Y[3] += Sin_mas[i+3];  
    } 
    
  int Xy=(X_Y[1]-X_Y[0])/2; 
  int Yy=(X_Y[2]-X_Y[3])/2;            
  delete[] Sin_mas;
  
  return max(abs(Xy),abs(Yy))+3*min(abs(Xy),abs(Yy))/8;  
}

void Timer2_irq1()
{      
      regs->CR2 |= ADC_CR2_SWSTART;
      while (!(regs->SR & ADC_SR_EOC));
      Sin_mas[Ared_count] = (uint16)(regs->DR & ADC_DR_DATA);
      
      regs->CR2 |= ADC_CR2_SWSTART;
      while (!(regs->SR & ADC_SR_EOC));
      Sin_mas[Ared_count]+= (uint16)(regs->DR & ADC_DR_DATA);
      
      regs->CR2 |= ADC_CR2_SWSTART;
      while (!(regs->SR & ADC_SR_EOC));
      Sin_mas[Ared_count] += (uint16)(regs->DR & ADC_DR_DATA);
      
      regs->CR2 |= ADC_CR2_SWSTART;
      while (!(regs->SR & ADC_SR_EOC));
      Sin_mas[Ared_count] += (uint16)(regs->DR & ADC_DR_DATA);
      
      Ared_count++;    
}

void Timer2_irq2()
{    
      regs->CR2 |= ADC_CR2_SWSTART;
      while (!(regs->SR & ADC_SR_EOC));
      Sin_mas[Ared_count] = (uint16)(regs->DR & ADC_DR_DATA);
      
      regs->CR2 |= ADC_CR2_SWSTART;
      while (!(regs->SR & ADC_SR_EOC));
      Sin_mas[Ared_count]+= (uint16)(regs->DR & ADC_DR_DATA);
      
      regs->CR2 |= ADC_CR2_SWSTART;
      while (!(regs->SR & ADC_SR_EOC));
      Sin_mas[Ared_count] += (uint16)(regs->DR & ADC_DR_DATA);
      
      regs->CR2 |= ADC_CR2_SWSTART;
      while (!(regs->SR & ADC_SR_EOC));
      Sin_mas[Ared_count] += (uint16)(regs->DR & ADC_DR_DATA);
      
      Ared_count++;     
}
//-----------------------------------------------Magnitude_mask()----------------------------------------------------------   
int Magnitude_mask(int Magn, int vdi)
{
  if (Mem.Mask==0)
  {
    if(vdi<-45)Magn = Magn*(-1*(pow(vdi+45,2)/2000)+1);   
    if(vdi>82)Magn = Magn*(1-((vdi-82)/8.0));      
  }
  
  if (Mem.Mask==1)Magn = Magn*(-1*(pow(vdi,2)/8000)+1);   
  
  if (Mem.Mask==2)
     {
      if(vdi>60) Magn = Magn*(1-((vdi-60)/30.0));
      if(vdi<-50) Magn = Magn*(1+((vdi- -50)/36.0));
     }
 return Magn;
}


//----------------------------------------------------profile_select()-----------------------------------------------------------------------------------------
void Prof_set(int index) 
{
  Mem.Prof=index;
  set_current_TX(Mem.amp_TX[Mem.Prof]);
  TX_conf();     
}

void profile_select()
{  
#define MENU_size 3 
static char* MENU_Strings[]=
{
"Prof#1",
"Prof#2",
"Prof#3", 
"Prof#4"
};

MENU(Mem.Prof, MENU_size, MENU_Strings , Prof_set); 
}
//--------------------------------------------Servis_ind---------------------------------------------------------------------
void Servis_ind()
{
lcd.clear();
lcd.setCursor(11,1); lcd.write(1);
lcd.setCursor(15,1); lcd.print('v');

while (true)
 {      

    lcd.setCursor(8,0);
    lcd.print("b=");
    lcd.print(ADC_Read_Data(true)/5600);//делителем калибруем показания баланса
    lcd.print("mV ");
    
   if( Button_read(2)) {Button_enter();  Mem.F_TX[Mem.Prof]++; Timer_2.setPrescaleFactor(Mem.F_TX[Mem.Prof]);}
   if( Button_read(4)) {Button_enter();  Mem.F_TX[Mem.Prof]--; Timer_2.setPrescaleFactor(Mem.F_TX[Mem.Prof]);}
   if( Button_read(5)) {Button_enter(); break;}
   check_button();
   
    lcd.setCursor(0,0);
    lcd.print(72000000/2/(Mem.F_TX[Mem.Prof]*SAMPLES_TX));
    lcd.print("Hz ");
        
    lcd.setCursor(0,1); 
    lcd.print(ADC_read(PA4)/current_cal); 
    lcd.print("mA  ");
   
    V_bat_ind();  
 }
  
}
 
//--------------------------------------------MASK---------------------------------------------------------
void Mask_Svitch()
{
 lcd.clear();
 while (true)
  { 
   
   if( Button_read(3)) {Button_enter(); Mem.Mask++; if (Mem.Mask>2)Mem.Mask=0; } 
   if( Button_read(5)) {Button_enter();  break;}//выход из цикла
   delay(100);
   check_button(); 
   
   lcd.setCursor(0,0);
   if (Mem.Mask==0)  lcd.print("Mask OFF     ");
   if (Mem.Mask==1)  lcd.print("Mask Parabola");
   if (Mem.Mask==2)  lcd.print("Mask Trapeze ");  
  }
} 

//---------------------------------------------CUT_VDI----------------------------------------------------------
void CUT_VDI()
{
   lcd.clear();
   lcd.setCursor(3,1); lcd.print("-");    
   lcd.setCursor(12,1); lcd.print("+");
   while (true)
    {     
      if( Button_read(2)) {Button_enter(); Mem.Cut_VDI_H[Mem.Prof]--;}
      if( Button_read(4)) {Button_enter(); Mem.Cut_VDI_H[Mem.Prof]++;}
      if( Button_read(5)) {Button_enter(); break;}
      delay(100);
      check_button(); 
      if( Mem.Cut_VDI_H[Mem.Prof]>90) Mem.Cut_VDI_H[Mem.Prof]=90;
      if( Mem.Cut_VDI_H[Mem.Prof]<45) Mem.Cut_VDI_H[Mem.Prof]=45;
      lcd.setCursor(0,0); 
      lcd.print("VDI high: +");
      lcd.print(Mem.Cut_VDI_H[Mem.Prof]);
    }   
}

//-------------------------------------------F_scan--------------------------------------------------------------------------
void F_scan()
{
  lcd.setCursor(0,0);lcd.print("Start scanning ?");
  lcd.setCursor(0,1);lcd.print(" YES         NO ");
  while (true)
  { 
   if( Button_read(1)) {Button_enter(); break;}  
   if( Button_read(5)) {Button_enter(); return;}
   check_button();
  }
  Button_FLAG=false;
  lcd.clear();
   
  Mem.amp_TX[Mem.Prof]=400;//Уровень TX 
  set_current_TX(Mem.amp_TX[Mem.Prof]);          
  Mem.Comp_Phase[Mem.Prof]= 0;//Установка фазы  0...37 
  Timer_2.setCompare(TIMER_CH2, Mem.Comp_Phase[Mem.Prof]);
  
  int i_max = 0;
  int mem_i;
  lcd.clear();
  lcd.setCursor(0,0);lcd.print("TX:"); 
  lcd.setCursor(0,1);lcd.print("resonance search"); 
  
  for (int i=260; i>50; i--)  
   {      
    Timer_2.setPrescaleFactor(i);
    delay(5);
    if( Button_read(5)) {Button_enter(); break;}
    int i_tx=ADC_read(PA4)/current_cal;
    if (i_tx>(i_max+2)){ i_max = i_tx; mem_i=i; lcd.setCursor(3,0);lcd.print(72000000/2/(i*SAMPLES_TX));}  
   } 

   Mem.F_TX[Mem.Prof]=mem_i;
   Timer_2.setPrescaleFactor(Mem.F_TX[Mem.Prof]);

  lcd.clear();
  BEEP(400);
  TX_current();
  Servis_ind();
  
}
//--------------------------------------------V_bat_ind-------------------------------------------------------------
void V_bat_ind()
{
static uint32_t V_tic,V_bat;

V_bat+=analogRead(PA0); 
V_tic++;
if (V_tic==50)
  {
    float bat=V_bat/19357.0;
    lcd.setCursor(12,1); lcd.print(bat,1);   
  
    if(bat<6.0)//&&V_cut>4
     {      
     digitalWrite(LED_LCD, LOW);    
     lcd.clear();
     lcd.print("Battery low");
     test_sound(2);    
     digitalWrite(MUTE, HIGH ); //MUTE ON
     PWR_BASE->CR |= PWR_CR_PDDS;
     SCB_BASE->SCR |= SCB_SCR_SLEEPDEEP;
     timer_disable_all();
     __asm volatile( "wfi" );// переход в режим SLEEP     
     }
   
    V_tic=0;
    V_bat=0;
  }  
}

//--------------------------------------------Ferit_seting-----------------------------------------------------------------
void  Ferit_seting()
{

int M=0,max_M=0;
int count=-60;
int vdi=0;
bool count_flag=false;
Xa=0;
Ya=0;

Mem.Comp_Phase[Mem.Prof]= 0;
Mem.Adjustment_Phase[Mem.Prof]=0;
Mem.Turn_phase[Mem.Prof]=1;// Переворот фазы -1
TX_conf();

Filter_set(1);
lcd.clear();
lcd.print("Wave ferrit");
delay(100);

while (true)
{
ADC_Read_Data(false);

if( Button_read(2)) {Button_enter();Mem.Comp_Phase[Mem.Prof]--;if(Mem.Comp_Phase[Mem.Prof]<0)Mem.Comp_Phase[Mem.Prof]=35; TX_conf(); count=-30;}
if( Button_read(3)) {Button_enter(); Mem.Turn_phase[Mem.Prof]*=-1; count=-30;}//if(vdi<90&&vdi>-90) Переворот фазы
if( Button_read(4)) {Button_enter();Mem.Comp_Phase[Mem.Prof]++;if(Mem.Comp_Phase[Mem.Prof]>35)Mem.Comp_Phase[Mem.Prof]=0; TX_conf(); count=-30;}
if( Button_read(5)) {Button_enter(); break;}
check_button();
lcd.setCursor(0,1);
lcd.print("Comp Phase: ");
lcd.print (Mem.Comp_Phase[Mem.Prof]);
lcd.print("  ");

int Ym=abs(Ya);
int Xm=abs(Xa);
M=max(Xm,Ym)+3*min(Xm,Ym)/8;

if(Error==false)
  { 
    
  if(M>2000&&count<40&&count>-1)
       { 
        if(M>max_M){max_M=M; vdi=round(degrees(atan2(Xa,Ya)));} 
        if(count_flag==false){Go_Sound(600,Max_amp); count_flag=true;}                
       }
  
  if ( count>40)
       {
         snd_OFF();
         
         max_M=0;
         count=0;  
         count_flag=false;
         
         lcd.setCursor(0,0);
         lcd.print("Real Phase:");
         if(vdi>=0)lcd.print("+"); 
         lcd.print(vdi);
         lcd.print(char(223));
         lcd.print("   ");                
       }  
     if (count_flag==true||count<0)count++; 
  }

else
    { 
     lcd.clear(); lcd.setCursor(3,0); lcd.print("ERROR");
     Go_Sound(200,Max_amp); delay(500); snd_OFF();  
    }  
}


lcd.clear(); 
lcd.print("Adj_Phase: ");
lcd.setCursor(3,1); lcd.print("-");    
lcd.setCursor(12,1); lcd.print("+");

max_M=0;
M=0;
Xa=0;
Ya=0;
Mem.Adjustment_Phase[Mem.Prof]= (vdi-Ferrit_VDI)*-1;

Sound_block=30;


while (true)
{
 
 if( Button_read(2)) {Button_enter(); Mem.Adjustment_Phase[Mem.Prof]--; Sound_block=30;}
 if( Button_read(4)) {Button_enter(); Mem.Adjustment_Phase[Mem.Prof]++; Sound_block=30;}
 if( Button_read(5)) {Button_enter(); break;}
 check_button();  
 
 lcd.setCursor(11,0); lcd.print(Mem.Adjustment_Phase[Mem.Prof]); lcd.print(char(223)); lcd.print(" ");
 
 ADC_Read_Data(false);
 int Ym=abs(Ya);
 int Xm=abs(Xa);
 M=max(Xm,Ym)+3*min(Xm,Ym)/8;

 vdi=round(degrees(atan2(Xa,Ya)))+Mem.Adjustment_Phase[Mem.Prof];
 
 if(M>2000 && Error==false && vdi<55 && vdi>-125)
 {
    if(M>max_M&&Sound_block<=0)
     {
       Go_Sound(600,Max_amp); 
       max_M=M;
       lcd.setCursor(5,1); 
       if(vdi>=0)lcd.print("+"); 
       lcd.print(vdi);
       lcd.print(char(223));
       lcd.print("  "); 
     }
    else{max_M=0;}
 }
else snd_OFF();
if(Sound_block>0)Sound_block--;
 }
}

//-----------------------------------------------phase_correction---------------------------------------------------------------
void phase_correction()
{
lcd.clear();   
lcd.setCursor(3,1); lcd.print("-");    
lcd.setCursor(12,1); lcd.print("+");
vdi_mem-=Mem.Adjustment_Phase[Mem.Prof];
while (true)
  { 
   
   lcd.setCursor(5,0);
   lcd.print("VDI");
   if((vdi_mem+Mem.Adjustment_Phase[Mem.Prof])>=0)lcd.print("+"); 
   lcd.print(vdi_mem+Mem.Adjustment_Phase[Mem.Prof]);
   lcd.print(" ");

   if( Button_read(2)) {Button_enter();Mem.Adjustment_Phase[Mem.Prof]--;}
   if( Button_read(4)) {Button_enter(); Mem.Adjustment_Phase[Mem.Prof]++;}
   if( Button_read(5)) {Button_enter(); break;}
   delay(100);
   check_button();  
  }

}
//---------------------------------------------Ground_balance------------------------------------------------------------
void Ground_balance()
{ 
  lcd.clear();
  BEEP(400);
  
  uint32_t Tic=0;
  float  K_LPF= 0.3;
  float  K_HPF= 0.1;
  int X_LPF=0, Y_LPF=0, X_old=0, Y_old=0, Xx=0, Yy=0;
  int Max_M=10000;
  int VDI;
 
  lcd.setCursor(0,0);
  lcd.print("G-vector: ");
  lcd.setCursor(0,1);
  lcd.print("G-level: ");
 
 while (true)
  {  
     
   ADC_Read_Data(true);    
   
   X_LPF+= (Xa - X_LPF) * K_LPF;
   Xx = (Xx +( X_LPF- X_old))*K_HPF ;
   X_old = X_LPF;

   Y_LPF+= (Ya - Y_LPF) * K_LPF;
   Yy = (Yy +( Y_LPF- Y_old))*K_HPF ;
   Y_old = Y_LPF;

   Tic++;
   if(Tic==60){ K_LPF=0.1; K_HPF= 0.6; }
   if( Tic>60)
    {
     VDI=round(degrees(atan2(Xx,Yy)))+Mem.Adjustment_Phase[Mem.Prof];
     int M = max(abs(Xx),abs(Yy))+3*min(abs(Xx),abs(Yy))/8;
     if(M>60 &&  VDI>-95 && VDI<-65) //
      {
       if(M>Max_M)
        {
          Max_M=M;
          Mem.Cut_VDI_L[Mem.Prof]=VDI;
          lcd.setCursor(10,0);
          lcd.print(Mem.Cut_VDI_L[Mem.Prof]);
          lcd.setCursor(10,1);
          lcd.print(Max_M/100);
          lcd.print("    ");
          Go_Sound( 200, Max_amp);       
          
        }
       else {snd_OFF();}
      }
    }
   
   if( Button_read(1)) {Button_enter(); return;}
   if( Button_read(2)) {Button_enter();  Mem.Cut_VDI_L[Mem.Prof]--;}
   if( Button_read(3)) {Button_enter(); Max_M=0;  lcd.setCursor(10,0); lcd.print("   "); lcd.setCursor(9,1);lcd.print("    "); snd_OFF();}
   if( Button_read(4)) {Button_enter();  Mem.Cut_VDI_L[Mem.Prof]++;}
   if( Button_read(5)) {Button_enter(); break;}
   check_button();  
   lcd.setCursor(10,0);
   lcd.print(Mem.Cut_VDI_L[Mem.Prof]);     
  }  
}

//------------------------------------------TX_current---------------------------------------------------------------------------
void TX_current()
{
  lcd.clear();
  lcd.setCursor(3,1); lcd.print("-");    
  lcd.setCursor(12,1); lcd.print("+");
  while (true)
  { 
   if( Button_read(1)) {Button_enter();break; }
   if( Button_read(2)) {Button_enter(); Mem.amp_TX[Mem.Prof]-=88; if(Mem.amp_TX[Mem.Prof]<0)Mem.amp_TX[Mem.Prof]=0; set_current_TX(Mem.amp_TX[Mem.Prof]);}  
   if( Button_read(3)) {Button_enter();break; }
   if( Button_read(4)) {Button_enter(); Mem.amp_TX[Mem.Prof]+=88; if(Mem.amp_TX[Mem.Prof]>1400)Mem.amp_TX[Mem.Prof]=1400; set_current_TX(Mem.amp_TX[Mem.Prof]);}  
   if( Button_read(5)) {Button_enter();break; }                                        
   delay(100);
   check_button(); 
      
    int curent_bar = Mem.amp_TX[Mem.Prof]/87; 
    lcd.setCursor(0,0);
    for (int i=0; i<curent_bar; i++){lcd.print(char(255));} // Индикация уровня сигнала
    for(int i=curent_bar; i<16; i++) lcd.print(' ');//clear
     
    lcd.setCursor(5,1); 
    lcd.print(ADC_read(PA4)/current_cal); 
    lcd.print("mA ");  
  } 
}
//----------------------------------------set_current_TX----------------------------------------------------------------------------
void set_current_TX(uint16_t  dac_output)//dac_output = 0...4095
{
  Wire.beginTransmission(DAC);
  Wire.write(0x60); 
  uint8_t firstbyte=(dac_output>>4);
  dac_output = dac_output << 12;  
  uint8_t secndbyte=(dac_output>>8);
  Wire.write(firstbyte); 
  Wire.write(secndbyte);
  Wire.endTransmission();
}


//=================================================Pin_Point======================================================================================
void Pin_Point()
{
static int PP_Threshold = 10;
lcd.clear(); 
lcd.print("VDI*** PP.Thr:");lcd.print(PP_Threshold); 
float kd;
int Xy,Yy;
int Old_X=0,Old_X1=0,Old_X2=0;
int Old_Y=0,Old_Y1=0,Old_Y2=0;
int Max_M=0;
int Tic=0;

micros_1 = micros();
while (true)
{ 
micros_2 = micros();
if( micros_2-micros_1>=25000)
{ 
 micros_1 = micros_2; 
           
   int  adc_data_1=0; 
   int  adc_data_2=0;
   
   while (digitalRead(DATA_pin_1) || digitalRead(DATA_pin_2));
   
            for (uint8_t i = 0; i < 24; i++) 
              {
                digitalWrite(CLOCK_pin, HIGH);
                delayMicroseconds(1);
                
                adc_data_1 <<= 1;
                if (digitalRead(DATA_pin_1)) adc_data_1 |= 1;
                
                adc_data_2 <<= 1;
                if (digitalRead(DATA_pin_2)) adc_data_2 |= 1;
                
                digitalWrite(CLOCK_pin, LOW);
                delayMicroseconds(1);               
                
              }          
                
              for (uint8_t i = 0; i < 2; i++) 
                {
                 digitalWrite(CLOCK_pin, HIGH);
                 delayMicroseconds(1);
                 digitalWrite(CLOCK_pin, LOW);
                 delayMicroseconds(1);
                }     
   
   adc_data_1*=Mem.Turn_phase[Mem.Prof];
   adc_data_2*=Mem.Turn_phase[Mem.Prof];                 
   
   int Xp=(adc_data_1+Old_X1)/2; Old_X1=Xp; // ФНЧ Xy
   int Yp=(adc_data_2+Old_Y1)/2; Old_Y1=Yp; // ФНЧ Yy    
  
 if(Tic<64){Tic++;kd=0.3;}
 if(Tic==60)kd=0.04;

 Xy=Xp;
 Yy=Yp;
 Xy-=Old_X;
 Yy-=Old_Y;
 Old_X+= (Xp - Old_X) * kd;
 Old_Y+= (Yp - Old_Y) * kd;

 Xy=(Xy+Old_X2)/2; Old_X2=Xy; // ФНЧ Xy
 Yy=(Yy+Old_Y2)/2; Old_Y2=Yy; // ФНЧ Yy  

 int VDI =round(degrees(atan2(Xy,Yy)))+Mem.Adjustment_Phase[Mem.Prof];
 int Magnitude = max(abs(Xy),abs(Yy))+3*min(abs(Xy),abs(Yy))/8;
 
 if( VDI<90 && VDI>-90 && Magnitude>PP_Threshold*16 && Tic>60)
    {   
      kd=0.02;       
      if(Magnitude>Max_M)
      {
       Max_M =Magnitude;
       lcd.setCursor(3,0);if(VDI>=0)lcd.print("+"); lcd.print(VDI); lcd.print(" ");
      }
      int map_S= map(VDI,-90,90, 40,256);           
      int Sound_PP=constrain((log10(Magnitude)*map_S), 50, 1200); 
      if(adc_data_1>ADC_max||adc_data_1<ADC_min||adc_data_2>ADC_max||adc_data_2<ADC_min){error_Sound(); Error=true;}
      else 
        { 
          Error=false; 
          if( VDI<=Mem.Cut_VDI_H[Mem.Prof]&&VDI>Mem.Cut_VDI_L[Mem.Prof])Go_Sound(Sound_PP,Max_amp);
          else snd_OFF();
        }        
      
      Magnitude = constrain((log10(Magnitude)*4)-8, 0, 16);
      lcd.setCursor(0,1); for(int i=0; i<Magnitude; i++)lcd.print(">");for(int i=Magnitude; i<16; i++)lcd.print(" ");  
    }
 else {kd=0.6; Max_M=0; snd_OFF(); lcd.setCursor(0,1); lcd.print("                ");}//
  
  if( check_button()==false){Button_FLAG=false;}
  if( Button_read(2)) {Button_enter(); PP_Threshold--; lcd.setCursor(13,0);lcd.print(PP_Threshold); lcd.print(" ");  }
  if( Button_read(3)) {Button_enter(); break;}
  if( Button_read(4)) {Button_enter(); PP_Threshold++; lcd.setCursor(13,0);lcd.print(PP_Threshold); lcd.print(" ");  }
}
}
Tic=0;
lcd.clear();  
}

//--------------------------------------------New_Menu_Item---------------------------------------------------------
void New_Menu_Item()
{
  lcd.clear();
  lcd.print("New Menu Item");
  lcd.setCursor(0,1);
  lcd.print("Press any key");
  
  while (true)
  { 
   if( Button_read(1)) {Button_enter(); break;}
   if( Button_read(2)) {Button_enter(); break;}
   if( Button_read(3)) {Button_enter(); break;}
   if( Button_read(4)) {Button_enter(); break;}
   if( Button_read(5)) {Button_enter(); break;}
   check_button();
   delay(100);
  }
}
  
