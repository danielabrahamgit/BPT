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

/* Sets the corresponding frequency bits */ 
void set_freq(float freq, float fref, bool approx) {

  int divider = set_divider(freq);
  
  unsigned int long INTA = (freq * divider) / fref;
  unsigned int long MOD;
  unsigned int long FRAC;
  
  double FRACF = 1.0*(freq * divider) / fref - INTA;
  
  /* fast approximation*/
  if (approx) { 
    MOD = (1 << 12) - 1;
    FRAC = (int) (1.0 * MOD * FRACF);
  } else {
    // search for closest fraction
    MOD = 1;
    FRAC = 0;
    unsigned int long minMOD = 1;
    unsigned int long minFRAC = 0;
    
    unsigned int long FRACD = FRACF;
    float diff;
    
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
  }

  /* Set frequenct related registers */
  registers[0] = 0;
  registers[0] = INTA << 15;
  FRAC = FRAC << 3;
  registers[0] = registers[0] + FRAC;
  registers[1] = 0;
  registers[1] = MOD << 3;
  registers[1] = registers[1] + 1;

  /* Sets prescaler to 8/9 */
  bitSet (registers[1], 27);
  
  /* Sets the MUXOUT to tell us what the digital lock is */ 
  bitSet (registers[2], 28);
  bitSet (registers[2], 27);
  bitClear (registers[2], 26);
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
 
  
