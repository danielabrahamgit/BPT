#include <SPI.h>

#define ADF4351_LE0 3
#define ADF4351_LE1 4

/* Represents the onboard registers */
uint32_t registers[6] = {0x4580A8, 0x80080C9, 0x4E42, 0x4B3, 0xBC8024, 0x580005};

/* Output power and internal reference clock */ 
unsigned RF_PWR = 3;
double F_REF = 25;

/* Output Frequency for each board */
double RFout0, RFout1;

/* Frequency sweep range, incrimentation value, and sweep indicator */
double rangeLow, rangeHigh, inc;
boolean manual = true;

/* Sweep Timing */
long prevtime = 0;
long curtime = 0;

/* Pilot tone frequency in MHz */
double pilot_freq = 0;//127.8;

/* Tells us if we should update the board */
boolean update = false;

void setup() {
  Serial.begin(9600);

  /* Set LE pins as outputs */
  pinMode(ADF4351_LE0, OUTPUT);
  pinMode(ADF4351_LE1, OUTPUT);

  /* Set LE pins */
  digitalWrite(ADF4351_LE0, HIGH);
  digitalWrite(ADF4351_LE1, HIGH);
  
  /* SPI init */
  SPI.begin();                        
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST); 

  /* Update the board's registers */
  update = true;

  user_interface();
  /*
  rangeLow = 100;
  rangeHigh = 120;
  inc = 1;
  manual = false;
  */
}

void loop() {
  /* Sweep or manual mode */
  if (!manual){
    long curtime = millis();
    if (curtime - prevtime > 500) {
      if (RFout0 > rangeHigh) {
        RFout0=rangeLow;
        RFout1=rangeLow + pilot_freq;
      } else {
        RFout0 += inc;
        RFout1 += inc;
      }
      update = true;
      prevtime = curtime;
    }
  }
  
  if (update) {
    int out_div = set_divider(RFout0);
    set_power(RF_PWR);
    set_freq(RFout0, out_div, F_REF);

    /* Sets prescaler to 8/9 */
    bitSet (registers[1], 27);

    /* Sets the MUXOUT to tell us what the digital lock is */ 
    bitSet (registers[2], 28);
    bitSet (registers[2], 27);
    bitClear (registers[2], 26);

    SetADF4351(0);
  }
  
  if (update) {
    int out_div = set_divider(RFout1);
    set_power(RF_PWR);
    set_freq(RFout1, out_div, F_REF);

    /* Sets prescaler to 8/9 */
    bitSet (registers[1], 27);
    
    /* Sets the MUXOUT to tell us what the digital lock is */ 
    bitSet (registers[2], 28);
    bitSet (registers[2], 27);
    bitClear (registers[2], 26);

    SetADF4351(1);
    update=false;
  }
}

/* Writes a single register onto board selected by LE */
void WriteRegister32(const uint32_t value, const int LE) {
  /* First, pull the Load Enable low */
  if (LE==0) {
    digitalWrite(ADF4351_LE0, LOW);
  } else {
    digitalWrite(ADF4351_LE1, LOW);
  }

  /* Next, we load in 32 bits */
  for (int i = 3; i >= 0; i--)
  SPI.transfer((value >> 8 * i) & 0xFF);

  /* Pull high then low again */
  if (LE==0) {
    digitalWrite(ADF4351_LE0, HIGH);
    digitalWrite(ADF4351_LE0, LOW);
  } else {
    digitalWrite(ADF4351_LE1, HIGH);
    digitalWrite(ADF4351_LE1, LOW);
  }
}

/* Writes all 6 registers via the registers global structure */ 
void SetADF4351(const int LE) {
  for (int i = 5; i >= 0; i--) 
  WriteRegister32(registers[i], LE);
}

/* Determines what kind of output we want */
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

/* Sets the corresponding frequency bits */ 
void set_freq(float freq, float divider, float fref) {
  unsigned int long INTA = (freq * divider) / fref;
  
  double FRACF = 1.0*(freq * divider) / fref - INTA;
  
  /* fast approximation*/ 
  unsigned int long MOD = (1 << 12) - 1;
  unsigned int long FRAC = (int) (1.0 * MOD * FRACF);
  
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
}

/* Sets the power bits */
void set_power(int pwr) {
  /* Clear Pwr bits */
  registers[4] &= ~(3 << 3);
  /* Set power level */
  registers[4] = registers[4] + (pwr<<3); 
}

/* Sets the frequency divider value */
int set_divider(float RFout) {
  int divider;
  if (RFout < 68.75) {
    divider = 64;
    bitWrite (registers[4], 22, 1);
    bitWrite (registers[4], 21, 1);
    bitWrite (registers[4], 20, 0);
  }
  else if (RFout < 137.5) {
    divider = 32;
    bitWrite (registers[4], 22, 1);
    bitWrite (registers[4], 21, 0);
    bitWrite (registers[4], 20, 1);
  }
  else if (RFout < 275)  {
    divider = 16;
    bitWrite (registers[4], 22, 1);
    bitWrite (registers[4], 21, 0);
    bitWrite (registers[4], 20, 0);
  }
  else if (RFout < 550)  {
    divider = 8;
    bitWrite (registers[4], 22, 0);
    bitWrite (registers[4], 21, 1);
    bitWrite (registers[4], 20, 1);
  }
  else if (RFout < 1100) {
    divider = 4;
    bitWrite (registers[4], 22, 0);
    bitWrite (registers[4], 21, 1);
    bitWrite (registers[4], 20, 0);
  }
  else if (RFout < 2200) {
    divider = 2;
    bitWrite (registers[4], 22, 0);
    bitWrite (registers[4], 21, 0);
    bitWrite (registers[4], 20, 1);
  }
  else if (RFout >= 2200) {
    divider = 1;
    bitWrite (registers[4], 22, 0);
    bitWrite (registers[4], 21, 0);
    bitWrite (registers[4], 20, 0);
  }
  return divider;
}

/*
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
    //registers[4] &= ~(3 << 3);

    registers[4] = registers[4] + (RFPWR<<3); // set power level
    
    INTA = (RFout1 * OutputDivider) / F_REF;

    // search for closest fraction
    MOD = 1;
    minMOD = 1;
    FRAC = 0;
    minFRAC = 0;
    
    FRACF = 1.0*(RFout1 * OutputDivider) / PFDRFout - INTA;

    // Alternative fast approximation 
    MOD = (1 << 12) - 1;
    FRAC = (int) (1.0 * MOD * FRACF);
    
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

  double fabs(double x)
{
  if (x < 0) {
    x = -x;
  }

  return x;
}
  */
 
  
