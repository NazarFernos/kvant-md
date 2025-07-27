
 
//------------------------------Функции сохранения настроек в EEPROM----------------------------

template <class T> int EEPROM_write(int ee, const T& value, int Dev)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
     {
      ee++; 
      *p++;
      if(eeprom_read_byte(ee, Dev)!= *p){ eeprom_write_byte(ee, *p, Dev);}          
     }
    return i;
}

//------------------------------------------------------------------------------------

template <class T> int EEPROM_read(int ee, T& value, int Dev)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++) {*p++ = eeprom_read_byte(ee++, Dev);}     
    return i;
}

//-----------------------------------------------------------------------------------

byte eeprom_read_byte(int eeaddress, int Dev) //Чтение байта из EEPROM 
 {
    byte rdata = 0;
    Wire.beginTransmission(Dev);
    Wire.write(eeaddress);
    Wire.endTransmission();
    Wire.requestFrom(Dev,1); 
    if (Wire.available()) rdata = Wire.read(); 
    return rdata;
  }
//----------------------------------------------------------------------------------
 void eeprom_write_byte(int eeaddress, byte data,int Dev) //Запись байта в EEPROM
  {
    int rdata = data;
    Wire.beginTransmission(Dev);
    Wire.write(eeaddress);
    Wire.write(rdata);
    Wire.endTransmission();
    delay(5);  
  }
//----------------------------------------------------------------------------------
void Str_Set()
{
 for (int i=0;i<num_prof;i++)
       {
        Mem.amp_TX[i]=300;       //Уровень TX  0...1400 
        Mem.F_TX[i]= 141;        //Частота TX = F_CPU/2/(Prescaler*SAMPLES)    
        Mem.Comp_Phase[i]= 0;  //Установка фазы  0...35
        Mem.Turn_phase[i]=1;   //переворот фазы -1       
        Mem.Adjustment_Phase[i]=0; //Калибровка показаний VDI                  
        Mem.Cut_VDI_H[i]= 86; // Отсечка горячих камней
        Mem.Cut_VDI_L[i]=-84;// Отсечка грунта
       }  
       Mem.Sound_schem=2;
       Mem.Prof = 0; //Текущий профиль 
       Mem.Threshold = 6 ;  // Порог отсечки сигналов
       Mem.vol_Sound=1;  //Уровень звука 
       Mem.Filter_select=2; //тип фильтра         
       Mem.LED_LCD=true;
       Mem.Mask=2; // Mask 1 Parabola 
}
//-------------------------------------------------------------------------------------
bool init_eeprom_mem()
{   
   bool flag=false; 
   int Tic=0; 
   byte _error =0; 
  
  Wire.beginTransmission(EEPROM);
  _error = Wire.endTransmission();
  if (_error !=0)
    {
      lcd.clear();
      lcd.print("EEPROM ERROR=");
      lcd.print(_error); 
      Str_Set(); 
      delay(2000);
      return false;
    }
  
  _error =0;
  Wire.beginTransmission(DAC);
  _error = Wire.endTransmission();
   if (_error !=0)
    {
      lcd.clear();
      lcd.print("MCP4725 ERROR=");
      lcd.print(_error);  
      delay(1000);
    }
 
 
 lcd.clear();
 while (digitalRead(button_input[0])==false) 
  { 
     delay(100);
     Tic++;
     lcd.print(char(255));
     if (Tic>16) 
       {                   
         Button_FLAG=true;
         BEEP(1000);
         lcd.clear();lcd.print("EEPROM Reset");
         flag=true;
         while (digitalRead(button_input[0])==false);        
         break;
       }  
  } 
    
  if (eeprom_read_byte(0,EEPROM) != 112 || flag==true) // данных нет, записываем по умолчанию
     {         
       Str_Set();       
       EEPROM_write(1, Mem, EEPROM);// Запись в EEPROM   
       delay(100);
       eeprom_write_byte(0, 112, EEPROM);// отметили наличие данных
       delay(100);
     }
return true;
}

//-------------------------------------------------------------------------------------------------------------------------

 
