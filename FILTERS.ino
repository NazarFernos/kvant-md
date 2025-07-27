
#define NCoef 2
//---------------------------------------F------------------------------------------------
/**************************************************************
Filter type: High Pass
Filter model: Bessel
Filter order: 2
Sampling Frequency: 80 Hz
Cut Frequency: 12 Hz
***************************************************************/
const float ACoef_F[NCoef+1] = {          
        0.38405672368075222000,
        -0.76811344736150444000,
        0.38405672368075222000          
    };

const float BCoef_F[NCoef+1] = {
       1.00000000000000000000,
        -0.42517944980726130000,
        0.11104744491574563000
    };
//-------------------------------------N--------------------------------------------
/**************************************************************
Filter type: High Pass
Filter model: Bessel
Filter order: 2
Sampling Frequency: 80 Hz
Cut Frequency: 8 Hz
***************************************************************/
const float ACoef_N[NCoef+1] = {
        0.59939452471411536000,
        -1.19878904942823070000,
        0.59939452471411536000
    };

const float BCoef_N[NCoef+1] = {
        1.00000000000000000000,
        -1.07222952866378570000,
        0.32534858487134488000
    };

//-------------------------------------S-------------------------------------------
/**************************************************************
Filter type: High Pass
Filter model: Bessel
Filter order: 2
Sampling Frequency: 80 Hz
Cut Frequency: 6 Hz
***************************************************************/
const float ACoef_S[NCoef+1] = {
        0.67867205598719094000,
        -1.35734411197438190000,
        0.67867205598719094000
    };

const float BCoef_S[NCoef+1] = {
        1.00000000000000000000,
        -1.27910915004047520000,
        0.43557775986534658000
    };
    
//---------------------------------iirFNS-------------------------------------------------------
float ACoef[NCoef+1];
float BCoef[NCoef+1];

float iir2A(float NewSample) 
{
    static float y[NCoef+1]; //output samples
    static float x[NCoef+1]; //input samples
    int n;

    for(n=NCoef; n>0; n--) { x[n] = x[n-1];y[n] = y[n-1];}
    x[0] = NewSample;
    
    y[0] = ACoef[0] * x[0];
    for(n=1; n<=NCoef; n++)y[0] += ACoef[n] * x[n] - BCoef[n] * y[n];        
    return y[0]*-1;
}

float iir2B(float NewSample) 
{
    static float y[NCoef+1]; //output samples
    static float x[NCoef+1]; //input samples
    int n;

    for(n=NCoef; n>0; n--) { x[n] = x[n-1];y[n] = y[n-1];}
    x[0] = NewSample;
    
    y[0] = ACoef[0] * x[0];
    for(n=1; n<=NCoef; n++)y[0] += ACoef[n] * x[n] - BCoef[n] * y[n];        
    return y[0]*-1;
}

//============================================================================================================
//-------------------------------------------------M1------------------------------------------------
/**************************************************************
Filter type: High Pass
Filter order: 1
Sampling Frequency: 80 Hz

***************************************************************/

#define Ntap_M 2 
float ACoef_M[Ntap_M];
float BCoef_M[Ntap_M];

//Butterworth: 6 Hz
const float ACoef_MS[Ntap_M] = {
         0.80640035423422929000,
        -0.80640035423422929000
    };
const float BCoef_MS[Ntap_M] = {
       1.00000000000000000000,
        -0.61280078815481065000
    };
 

//Butterworth: 12Hz
const float ACoef_MN[Ntap_M] = {
        0.66245984812710823000,
        -0.66245984812710823000
    };
const float BCoef_MN[Ntap_M] = {
         1.00000000000000000000,
        -0.32491969625421685000
    };


float iir1A_M(float NewSample) {
    
    static float y[2]; //output samples
    static float x[2]; //input samples

       x[1] = x[0];
       y[1] = y[0];
       x[0] = NewSample;
   
    y[0] = ACoef_M[0] * x[0];
    y[0] += ACoef_M[1] * x[1] - BCoef_M[1] * y[1];
    
    return y[0];
}

float iir1B_M(float NewSample) {
    
    static float y[2]; //output samples
    static float x[2]; //input samples

       x[1] = x[0];
       y[1] = y[0];
       x[0] = NewSample;
   
    y[0] = ACoef_M[0] * x[0];
    y[0] += ACoef_M[1] * x[1] - BCoef_M[1] * y[1];
    
    return y[0];
}


//----------------------------------LPF----------------------------------------------------------------------
/**************************************************************
Filter type: Low Pass iir
Filter model: Bessel
Filter order: 2
Sampling Frequency: 80 Hz
Cut Frequency: 12 Hz
***************************************************************/

#define NCoef 2
 float ACoef_L[NCoef+1] = {
        0.12119477042068036000,
        0.24238954084136072000,
        0.12119477042068036000
    };

    float BCoef_L[NCoef+1] = {
        1.00000000000000000000,
        -0.69125618725985949000,
        0.17603526894189717000
    };

float iir_AL(float NewSample) {
   
    static float y[NCoef+1]; //output samples
    static float x[NCoef+1]; //input samples
    int n;

    //shift the old samples
    for(n=NCoef; n>0; n--) 
    {
       x[n] = x[n-1];
       y[n] = y[n-1];
    }

    //Calculate the new output
    x[0] = NewSample;
    y[0] = ACoef_L[0] * x[0];
    for(n=1; n<=NCoef; n++) y[0] += ACoef_L[n] * x[n] - BCoef_L[n] * y[n];
    
    return y[0];
}

float iir_BL(float NewSample) {
   
    static float y[NCoef+1]; //output samples
    static float x[NCoef+1]; //input samples
    int n;

    //shift the old samples
    for(n=NCoef; n>0; n--) 
    {
       x[n] = x[n-1];
       y[n] = y[n-1];
    }

    //Calculate the new output
    x[0] = NewSample;
    y[0] = ACoef_L[0] * x[0];
    for(n=1; n<=NCoef; n++) y[0] += ACoef_L[n] * x[n] - BCoef_L[n] * y[n];
    
    return y[0];
}

//-------------------------------------Filter_set()-----------------------------------
void Filter_set(int F )
{
 
 for(int i=0; i<=Ntap_M-1; i++)
  {
   if(F==0){ACoef_M[i]=ACoef_MN[i]; BCoef_M[i]=BCoef_MN[i];}
   if(F==4){ACoef_M[i]=ACoef_MS[i]; BCoef_M[i]=BCoef_MS[i];}         
  }
 
 for(int i=0; i<=NCoef; i++)
  {
   if( F ==1){ACoef[i]=ACoef_S[i]; BCoef[i]=BCoef_S[i];}
   if( F ==2){ACoef[i]=ACoef_N[i]; BCoef[i]=BCoef_N[i];}  
   if( F ==3){ACoef[i]=ACoef_F[i]; BCoef[i]=BCoef_F[i];}         
  }
}

//------------------------------------------------Filter_select-----------------------------------------------------

void Filter_select()
{   
 int Tic=0;
 lcd.clear();    
 lcd.setCursor(0,0); lcd.print("<Filter select>");
 lcd.setCursor(0,1); lcd.print("F");
 lcd.setCursor(3,1); lcd.print("N");
 lcd.setCursor(7,1); lcd.print("S"); 
 lcd.setCursor(11,1);lcd.print("M");  
 lcd.setCursor(14,1);lcd.print("U"); 

 lcd.blink(); 
 if(Mem.Filter_select==3)lcd.setCursor(0,1);//F
 if(Mem.Filter_select==2)lcd.setCursor(3,1);//N 
 if(Mem.Filter_select==1)lcd.setCursor(7,1);//S 
 if(Mem.Filter_select==0)lcd.setCursor(11,1);//M
 if(Mem.Filter_select==4)lcd.setCursor(14,1);//U

while (true)
 {    
     
   if( Button_read(1)) {Button_enter(); Mem.Filter_select=3; Otsechka1Sempla=0; break;}//F
   if( Button_read(2)) {Button_enter(); Mem.Filter_select=2; Otsechka1Sempla=1; break;}//N 
   if( Button_read(3)) {Button_enter(); Mem.Filter_select=1; Otsechka1Sempla=1; break;}//S 
   if( Button_read(4)) {Button_enter(); Mem.Filter_select=0; Otsechka1Sempla=1; break;}//M
   if( Button_read(5)) {Button_enter(); Mem.Filter_select=4; Otsechka1Sempla=1; break;}//m 
   delay(100);
   check_button();
 }
 lcd.noBlink();
 lcd.clear();
}
