//   ADF4351 with fixed frequency
//   Code modified from original code By Alain Fort F1CJN march 9,2019
//   
//
//
//  *************************************************** ENGLISH ***********************************************************
//  This software is used to programm an ADF4351 with a fixed frequency, using a 10 MHz reference frequency.
//  The frequency can be changed at line 80, using the same format.
//  The reference frquency can be changed at line 70, using the same format (Default 10 MHz)
//  ******************************************** HARDWARE IMPORTANT********************************************************
//  With an Arduino UN0 : uses a resistive divider to reduce the voltage, MOSI (pin 11) to
//  ADF DATA, SCK (pin13) to ADF CLK, Select (PIN 3) to ADF LE
//  Resistive divider 560 Ohm with 1000 Ohm to ground on Arduino pins 11, 13 et 3 to adapt from 5V
//  to 3.3V the digital signals DATA, CLK and LE send by the Arduino.
//  Arduino pin 2 (for lock detection) directly connected to ADF4351 card MUXOUT.
//  The ADF card is 5V powered by the ARDUINO (PINs +5V and GND are closed to the Arduino LED).


#include <SPI.h>
#define ADF4351_LE0 3
#define ADF4351_LE1 4
#define PLLOCK0 7
#define PLLOCK1 8

uint32_t registers[6] =  {0x4580A8, 0x80080C9, 0x4E42, 0x4B3, 0xBC8024, 0x580005} ; // 437 MHz avec ref à 25 MHz


int address,modif=0;
unsigned int i = 0;
double RFout0, REFin, INT, PFDRFout,  FRACF, RFACT, FRACD, diff;
double RFoutMin = 35, RFoutMax = 4400, REFinMax = 250, PDFMax = 32;
double RFout1 = RFoutMax;
unsigned int long RFint,RFintold,INTA,RFcalc,PDRFout, MOD, FRAC, RFPWR, minMOD, minFRAC;
byte OutputDivider;byte lock=2;
unsigned int long reg0, reg1;
boolean manual = true;
double rangeLow, rangeHigh;
double pilot_freq = 127.8;
double inc = 1;
long prevtime = 0;

int pllock0, pllock1;

void WriteRegister32(const uint32_t value, const int LE)   //Programme un registre 32bits
{
  if (LE==0) {
    digitalWrite(ADF4351_LE0, LOW);
  }else{
    digitalWrite(ADF4351_LE1, LOW);
  }
  for (int i = 3; i >= 0; i--)          // boucle sur 4 x 8bits
  SPI.transfer((value >> 8 * i) & 0xFF); // décalage, masquage de l'octet et envoi via SPI
  if (LE==0) {
    digitalWrite(ADF4351_LE0, HIGH);
    digitalWrite(ADF4351_LE0, LOW);
  }else{
    digitalWrite(ADF4351_LE1, HIGH);
    digitalWrite(ADF4351_LE1, LOW);
  }
}

void SetADF4351(const int LE)  // Programme tous les registres de l'ADF4351
{ for (int i = 5; i >= 0; i--)  // programmation ADF4351 en commencant par R5
    WriteRegister32(registers[i], LE);
}

void user_interface() {
  Serial.println("Please select which mode you would like:\n");
  Serial.println("'s' - Sweep through frequencies in a defined range");
  Serial.println("'m' - Manually Select both frequencies\n");
  while (Serial.available() == 0);
  char inp = Serial.read();
  while (Serial.available() != 0) Serial.read();
  if (inp == 's') {
    Serial.print("Enter the low frequency(MHz): ");
    do {
      rangeLow = Serial.parseFloat();
    } while(rangeLow < 0.1);
    Serial.println(rangeLow);
    
    Serial.print("Enter the high frequency(MHz): ");
    do {
      rangeHigh = Serial.parseFloat();
    } while(rangeHigh < 0.1);
    Serial.println(rangeHigh);
    
    Serial.print("Enter the frequency incrementation(MHz): ");
    do {
      inc = Serial.parseFloat();
    } while(inc < 0.1);
    Serial.println(inc);
    manual = false;
  } else if (inp == 'm') {
    Serial.print("Enter frequency 1(MHz): ");
    do {
      RFout0 = Serial.parseFloat();
    } while(RFout0 < 0.1);
    Serial.println(RFout0);
    
    Serial.print("Enter frequency 2(MHz): ");
    do {
      RFout1 = Serial.parseFloat();
    } while(RFout1 < 0.1);
    Serial.println(RFout1);
    manual = true;
  } else {
    Serial.println('Invalid input, try again');
    user_interface();
  }
}


//************************************ Setup ****************************************
  void setup() {
  Serial.begin (9600); 					//  Serial to the PC via Arduino "Serial Monitor"  at 9600

  pinMode(PLLOCK0, INPUT);  			// PIN 7 en entree pour lock
  pinMode(PLLOCK1, INPUT);  // PIN 7 en entree pour lock
  
  pinMode(ADF4351_LE0, OUTPUT);          // Setup pins
  pinMode(ADF4351_LE1, OUTPUT);          // Setup pins
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(ADF4351_LE0, HIGH);
  digitalWrite(ADF4351_LE1, HIGH);
  SPI.begin();                          // Init SPI bus
  SPI.setDataMode(SPI_MODE0);           // CPHA = 0 et Clock positive
  SPI.setBitOrder(MSBFIRST);            // poids forts en tête

  pllock0 = 0;
  pllock1 = 0;
  modif = 1;

  user_interface();
} // Fin setup

void loop()
{
 //********************************************** 
  if (!manual){
    long curtime = millis();
    if (curtime - prevtime > 100) {
      if (RFout1 > rangeHigh) {
        RFout0=rangeLow;
        RFout1=rangeLow + pilot_freq;
      } else {
        RFout0 += inc;
        RFout1 += inc;
      }
      modif = 1;
      prevtime = curtime;
    }
  }
  RFPWR = 0; // 0->3
  PFDRFout=10; // Frequence de reference
 //********************************************
  
  if  (modif==1) {
    
    if (RFout0 >= 2200) {
      OutputDivider = 1;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout0 < 2200) {
      OutputDivider = 2;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout0 < 1100) {
      OutputDivider = 4;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout0 < 550)  {
      OutputDivider = 8;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout0 < 275)  {
      OutputDivider = 16;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout0 < 137.5) {
      OutputDivider = 32;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout0 < 68.75) {
      OutputDivider = 64;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 0);
    }
    
    /* Potential Fix 
    // Clear Pwr bits
    registers[4] &= ~(3 << 3); */
    registers[4] = registers[4] + (RFPWR<<3); // set power level
    
    INTA = (RFout0 * OutputDivider) / PFDRFout;
    
    // search for closest fraction
    MOD = 1;
    minMOD = 1;
    FRAC = 0;
    minFRAC = 0;
    
    FRACF = 1.0*(RFout0 * OutputDivider) / PFDRFout - INTA;
    
    /* Alternative fast approximation */
    // MOD = (1 << 12) - 1;
    // FRAC = (int) (1.0 * MOD * FRACF);
    
    FRACD = FRACF;

    /* Dear god */
    while(MOD < 4096)
    {
      diff = 1.0*FRAC/MOD - FRACF;
      if (fabs(diff) < FRACD)
      {
        FRACD = fabs(diff);
        minMOD = MOD;
        minFRAC = FRAC;
      }
      if (diff == FRACF) {
        break;
      } else if (diff < 0)
      {
        FRAC = FRAC + 1;
      } else
      {
        MOD = MOD + 1;
      }

      }
    
    MOD = minMOD;
    FRAC = minFRAC;
    
    
    RFACT = 1.0*PFDRFout/OutputDivider * (INTA + (1.0*FRAC/MOD));

    Serial.print(" PLL0:  ");
    Serial.print(" Power Level: ");
    Serial.print(RFPWR);
    Serial.print(" Mod: ");
    Serial.print(MOD);
    Serial.print(" Frac: ");
    Serial.print(FRAC);
    Serial.print(" INT: ");
    Serial.print(INTA);
    Serial.print(" R: ");
    Serial.print(OutputDivider);
    Serial.print(" RFACT: ");
    Serial.print(RFACT,5);
    Serial.print("\n");

    
    registers[0] = 0;
    registers[0] = INTA << 15; // OK
    FRAC = FRAC << 3;
    /* This might be an issue because we will accumulate... */
    registers[0] = registers[0] + FRAC;
    /* Fix: */
    //registers[0] = registers[0] & ( + FRAC;

    registers[1] = 0;
    registers[1] = MOD << 3;
    registers[1] = registers[1] + 1 ; // ajout de l'adresse "001"
    bitSet (registers[1], 27); // Prescaler sur 8/9

    bitSet (registers[2], 28); // Digital lock == "110" sur b28 b27 b26
    bitSet (registers[2], 27); // digital lock 
    bitClear (registers[2], 26); // digital lock

    SetADF4351(0);  // Programme tous les registres de l'ADF4351
    
  }


  if (modif==1) {
    
    if (RFout1 >= 2200) {
      OutputDivider = 1;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout1 < 2200) {
      OutputDivider = 2;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout1 < 1100) {
      OutputDivider = 4;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout1 < 550)  {
      OutputDivider = 8;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout1 < 275)  {
      OutputDivider = 16;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout1 < 137.5) {
      OutputDivider = 32;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout1 < 68.75) {
      OutputDivider = 64;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 0);
    }

    /* Potential Fix 
    // Clear Pwr bits
    registers[4] &= ~(3 << 3);
    */
    registers[4] = registers[4] + (RFPWR<<3); // set power level
    
    INTA = (RFout1 * OutputDivider) / PFDRFout;

    // search for closest fraction
    MOD = 1;
    minMOD = 1;
    FRAC = 0;
    minFRAC = 0;
    
    FRACF = 1.0*(RFout1 * OutputDivider) / PFDRFout - INTA;

    /* Alternative fast approximation */
    // MOD = (1 << 12) - 1;
    // FRAC = (int) (1.0 * MOD * FRACF);
    
    FRACD = FRACF;
    
    while(MOD < 4096)
    {
        
        diff = 1.0*FRAC/MOD - FRACF;

        if (fabs(diff) < FRACD)
        {
          FRACD = fabs(diff);
          minMOD = MOD;
          minFRAC = FRAC;
        }
        
        if (diff == FRACF) {
          break;
        } else if (diff < 0)
        {
          FRAC = FRAC + 1;
        } else
        {
          MOD = MOD + 1;
        }

      }

    MOD = minMOD;
    FRAC = minFRAC;
    
    

    RFACT = 1.0*PFDRFout/OutputDivider * (INTA + (1.0*FRAC/MOD));

    Serial.print(" PLL1:  ");
    Serial.print(" Power Level: ");
    Serial.print(RFPWR);
    Serial.print(" Mod: ");
    Serial.print(MOD);
    Serial.print(" Frac: ");
    Serial.print(FRAC);
    Serial.print(" INT: ");
    Serial.print(INTA);
    Serial.print(" R: ");
    Serial.print(OutputDivider);
    Serial.print(" RFACT: ");
    Serial.print(RFACT,5);
    Serial.println("\n");
    
    registers[0] = 0;
    registers[0] = INTA << 15; // OK
    FRAC = FRAC << 3;
    registers[0] = registers[0] + FRAC;

    registers[1] = 0;
    registers[1] = MOD << 3;
    registers[1] = registers[1] + 1 ; // ajout de l'adresse "001"
    bitSet (registers[1], 27); // Prescaler sur 8/9

    bitSet (registers[2], 28); // Digital lock == "110" sur b28 b27 b26
    bitSet (registers[2], 27); // digital lock 
    bitClear (registers[2], 26); // digital lock
   
    SetADF4351(1);  // Programme tous les registres de l'ADF4351
    modif=0;
 
  }

   // Maybe this caused the bug!
   /*
   if (pllock0 == 0) {
    pllock0 = digitalRead(PLLOCK0);
    //Serial.print(".");
    if (pllock0) Serial.print(" PLL0 Locked! ");
    
  }

  if (pllock1 == 0) {
    pllock1 = digitalRead(PLLOCK1);
    //Serial.println(".");
    if (pllock1) Serial.print(" PLL1 Locked! ");
    
  }
  */
    
}   // fin loop


double fabs(double x)
{
  if (x < 0) {
    x = -x;
  }

  return x;
}

 
  
