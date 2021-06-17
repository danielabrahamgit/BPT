#define MIN_FLOAT 0.01

/* Determines what kind of output we want */
void user_interface() {
  /* Clear space */
  for (int i = 0; i < 20; i++)
    Serial.println();
    
  /* Starting scren */
  Serial.println("Please select which mode you would like:\n");
  Serial.println("'s' - Sweep through frequencies in a defined range");
  Serial.println("'m' - Manually Select both frequencies\n");

  /* Read output type */
  while (Serial.available() == 0);
  char inp = Serial.read();
  while (Serial.available() != 0) Serial.read();
  /* Sweep mode */
  if (inp == 's') {
    Serial.print("Enter the low frequency(MHz): ");
    do {
      rangeLow = Serial.parseFloat();
    } while(rangeLow < MIN_FLOAT);
    Serial.println(rangeLow);
    
    Serial.print("Enter the high frequency(MHz): ");
    do {
      rangeHigh = Serial.parseFloat();
    } while(rangeHigh < MIN_FLOAT);
    Serial.println(rangeHigh);
    
    Serial.print("Enter the frequency incrementation(MHz): ");
    do {
      inc = Serial.parseFloat();
    } while(inc < MIN_FLOAT);
    Serial.println(inc);
    manual = false;
    RFout0 = rangeLow;
    RFout1 = rangeLow + pilot_freq;
  } 
  /* Manual mode */
  else if (inp == 'm') {
    Serial.print("Enter frequency 1(MHz): ");
    do {
      RFout0 = Serial.parseFloat();
    } while(RFout0 < MIN_FLOAT);
    Serial.println(RFout0);
    
    Serial.print("Enter frequency 2(MHz): ");
    do {
      RFout1 = Serial.parseFloat();
    } while(RFout1 < MIN_FLOAT);
    Serial.println(RFout1);
    manual = true;
  } else {
    Serial.println('Invalid input, try again');
    user_interface();
  }

  /* Noise mode select */
  Serial.print("Noise mode - 'n' = low noise, 's' = low spur: ");
  do {
    inp = Serial.read();
  } while (inp != 'n' and inp != 's');
  Serial.println(inp);
  if (inp == 's') {
    init_state[2] |= NOISE_SELECT << 29;
  } else if (inp == 'n') {
    init_state[2] &= ~(NOISE_SELECT << 29);
  } else {
    Serial.println('Invalid input, try again');
    user_interface();
  }
}

void check_reset() {
  char inp = Serial.read();
//  do {
//    inp = Serial.read();
//  } while (!isAlphaNumeric(inp));
  if (inp == 'r') {
    /* Set all registers */
    for (int i = 0; i < 6; i++) {
      registers[i] = init_state[i];
    }
  
    /* Write registers */
    SetADF4351(0);
    SetADF4351(1);
    resetFunc();
  }
}
