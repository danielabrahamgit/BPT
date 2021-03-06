#include <SPI.h>

#define ADF4351_LE0 3
#define ADF4351_LE1 4
#define TIME_PER_FREQ 200

#define NOISE_SELECT 3

/* resets the arduino */
void(* resetFunc) (void) = 0;

/* Startup state for the registers */
uint32_t init_state[6] = {0x4580A8, 0x80080C9, 0x4E42, 0x4B3, 0xBC8024, 0x580005};

/* Represents the onboard registers */
uint32_t registers[6];

/* Output power and internal reference clock */ 
unsigned RF_PWR = 3;
double F_REF = 10;

/* Output Frequency for each board */
double RFout0, RFout1;

/* Frequency sweep range, incrimentation value, and sweep indicator */
double rangeLow, rangeHigh, inc;
boolean manual = true;

/* Sweep Timing */
long prevtime = 0;
long curtime = 0;

/* Pilot tone frequency in MHz */
double pilot_freq = 127.8;

/* Tells us if we should update the board */
boolean update_board = false;

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
  update_board = true;

  /* Uses serial port to interact with board */
  user_interface();

  /* Set all registers */
  for (int i = 0; i < 6; i++) {
    registers[i] = init_state[i];
  }
}

void loop() {
  /* Check for reset command */
  check_reset();
  
  /* Sweep or manual mode */
  if (!manual){
    long curtime = millis();
    if (curtime - prevtime > TIME_PER_FREQ) {
      if (RFout0 > rangeHigh) {
        RFout0=rangeLow;
        RFout1=rangeLow + pilot_freq;
      } else {
        RFout0 += inc;
        RFout1 += inc;
        Serial.print("RFOUT0: ");
        Serial.print(RFout0);
        Serial.print("\t\t RFOUT1: ");
        Serial.print(RFout1);
        Serial.print("\t\t Divider0: ");
        Serial.println(get_divider(RFout0));
      }
      update_board = true;
      prevtime = curtime;
    }
  }

  /* Updates the first board with new frequency specified 
   * by RFout0, and power output specified by RF_PWR.
   */
  if (update_board) {
    set_power(RF_PWR);
    set_freq(RFout0, F_REF, true);
    SetADF4351(0);
  }
  /* Updates the second board with new frequency specified 
   * by RFout0, and power output specified by RF_PWR.
   */
  if (update_board) {
    set_power(RF_PWR);
    set_freq(RFout1, F_REF, true);
    SetADF4351(1);
    update_board=false;
  }
}
