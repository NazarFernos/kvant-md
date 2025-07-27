//==============================================Button============================================================

void Button_CLK(int button_n)
{
  snd_OFF();
  Button_FLAG=true;
  int Tic=0;
  lcd.clear();
  
 while (digitalRead(button_input[0])==false && button_n!=0) 
  { 
     delay(100);
     Tic++;
     lcd.print(char(255));
     if (Tic>10) 
       {                   
         BEEP(400);
         while (digitalRead(button_input[0])==false);  
         phase_correction();
         button_n=0;
         break;
       }  
  }
 
 
 while (digitalRead(button_input[2])==false && button_n!=0) 
  { 
     delay(100);
     Tic++;
     lcd.print(char(255));
     if (Tic>10) 
       {                   
         BEEP(400);
         while (digitalRead(button_input[2])==false);  
         Filter_select(); 
         button_n=0;
         break;
       }  
  }
  
 while (digitalRead(button_input[3])==false && button_n!=0) 
  { 
     delay(100);
     Tic++;
     lcd.print(char(255));
     if (Tic>10) 
       {                   
         BEEP(400);
         while (digitalRead(button_input[3])==false);  
         sound_level(); 
         button_n=0;
         break;
       }  
  }
  
 while (digitalRead(button_input[4])==false && button_n!=0) //LED_LCD ON/OFF
  { 
     delay(100);
     Tic++;
     lcd.print(char(255));
     if (Tic>10) 
       {                   
         lcd.clear();
         
         digitalWrite(LED_LCD, !digitalRead(LED_LCD)); 
         if(digitalRead(LED_LCD)){Mem.LED_LCD=true;lcd.print("LED ON"); } 
         else {Mem.LED_LCD=false; lcd.print("LED OFF");}
         while (digitalRead(button_input[4])==false);   
         button_n=0;
         break;
       }  
  }
  
  
  if(button_n==1){ Ground_balance();}
  if(button_n==2){Button_enter(); Mem.Threshold--; if(Mem.Threshold < 1)Mem.Threshold=1;}
  if(button_n==3){Button_enter(); Pin_Point();}
  if(button_n==4){Button_enter(); Mem.Threshold++; if(Mem.Threshold > 64)Mem.Threshold=64;}
  if(button_n==5){Button_enter(); Menu();}
  
  Wire.beginTransmission(EEPROM);
  if (Wire.endTransmission()==0) EEPROM_write(1, Mem, EEPROM); // Запись в EEPROM - Сохранение настроек   
  LCD_start(); 
}
//-----------------------------------------------------------------------------------------------------------
void Button_enter()
{
 BEEP(400);Button_FLAG=true;
}
//----------------------------------------------------------------------------------------------------------
bool Button_read(int But)
{
 return(digitalRead(button_input[But-1])==false&&Button_FLAG==false)?true:false;
}
//---------------------------------------------------------------------------------------------------------
int check_button()// Проверка если нажатая кнопка
{
 int n=0;
 for(int i=0; i<5; i++)if(digitalRead(button_input[i])==false)n=i+1; 
 if (n==0)Button_FLAG=false;
 return n;
}

//-----------------------------------------MENU_entr-----------------------------------------------------------------------------------
void MENU_entr(int index)
{
    if (index==0) sound_level();// SOUND VOL
    if (index==1) profile_select();// PROFILE   
    if (index==2) Servis_ind();// SERVIS IND
    if (index==3) Filter_select();//
    if (index==4) Mask_Svitch();
    if (index==5) CUT_VDI();
    if (index==6) TX_current();//COIL_CURRENT
    if (index==7) F_scan();// RESONACE 
    if (index==8) Ferit_seting();
    if (index==9) New_Menu_Item();// НОВИЙ ПУНКТ МЕНЮ

}

//-------------------------------------Menu()-------------------------------------------------------------------------------------
void Menu()
{
#define MENU_size 9 
static int MENU_index = 1;

static char* MENU_Strings[]=
{
"1# SOUND VOL",
"2# PROFILE",
"3# SERVIS IND", 
"4# FILTR SEL",
"5# MASK",
"6# CUT VDI",
"7# TX CURRENT",
"8# RESONACE",
"9# FERRIT SET",
"10# NEW ITEM",
};

int N_MENU = MENU(MENU_index, MENU_size, MENU_Strings , MENU_entr);
MENU_index=N_MENU-1;
}

//----------------------------------------------------------------------------------------------------------


int MENU (int index, int size, char* Strings[], void (*F)(int) ) //index - положение в меню, size - размер меню, Strings[] массив строк меню, *F - передаваемая функция:    
{   
   bool up_down_flag=true; // Флаг положения курсора
   bool change_flag=true;//Флаг изменения меню
   int R_index=index+1;      
   lcd.clear();lcd.print('>');
   
  while (true)
   {
            
     if( check_button()==false){Button_FLAG=false;}
     
     if( Button_read(2)) //v
       {
         Button_enter();                  
         lcd.noBlink();
         if(up_down_flag==true)
         {     
          lcd.setCursor(0,0);lcd.print(" ");
          lcd.setCursor(0,1);lcd.print(">");
          R_index--; 
         }               
         else {index--;  R_index--; }        
         up_down_flag=false;
         if( R_index==0)R_index=size+1;
         if(index<0) {index=size; R_index=size;}
         change_flag=true;                
       }
     
     if( Button_read(3)) {Button_enter(); lcd.noBlink(); if((*F)!= NULL) (*F)(R_index-1); break;}
     
     if( Button_read(4)) //^
       {
         Button_enter();
         lcd.noBlink();
         if( R_index==size+1)R_index=0;
         if(up_down_flag==false)
         {        
          lcd.setCursor(0,0);lcd.print(">");
          lcd.setCursor(0,1);lcd.print(" ");  
          R_index++;  
         }          
         else {index++; R_index++;}
         up_down_flag=true;        
         if(index>size){index=0;R_index=1;}
         change_flag=true;
       }   
     
     if( Button_read(5)) {Button_enter(); lcd.noBlink(); lcd.clear(); break;}
      
     if(change_flag==true)
      {                     
       lcd.setCursor(1,0); lcd.print("               ");
       lcd.setCursor(1,1); lcd.print("               ");    
       lcd.setCursor(1,0); lcd.print(Strings[index]);       
       lcd.setCursor(1,1); if(index==0)lcd.print(Strings[size]); else lcd.print(Strings[index-1]); 
       change_flag=false;
      }          
       
       if(up_down_flag==true)lcd.setCursor(0,0); 
       else lcd.setCursor(0,1);      
       lcd.blink();    
       
       delay(100);   
    }
 return R_index;
}


//====================================================================================================================================

void  LCD_start()
{ 
 lcd.clear();
 lcd.print("VDI***    Thr:"); lcd.print(Mem.Threshold); 
 lcd.setCursor(11,1); lcd.write(1); 
 lcd.setCursor(15,1); lcd.print('v');
 
 lcd.setCursor(9,1);
 if(Mem.Mask==0)lcd.print("n");
 if(Mem.Mask==1)lcd.print("p");
 if(Mem.Mask==2)lcd.print("t");
 
 lcd.setCursor(8,1);
 if ( Mem.Filter_select==4){lcd.print("U"); }
 if ( Mem.Filter_select==0){lcd.print("M"); }
 if ( Mem.Filter_select==1){lcd.print("S"); }
 if ( Mem.Filter_select==2){lcd.print("N"); }
 if ( Mem.Filter_select==3){lcd.print("F"); }
 Filter_set(Mem.Filter_select);    
 
 lcd.setCursor(0,1);
 for(int i=0; i<7; i++){lcd.write('<'); }
 Sound_block=30;
}
