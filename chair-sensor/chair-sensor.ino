/*******************************************************************************

 Chair Sensor
 ------------------------------
 Detects a user sitting in a chair and then plays a sound file continuously
 until they leave. If they reach out and touch a large capacitive object,
 like another person, the sound changes.

 Written by Becky Stewart, 2015

 Adapted from DataStream.ino by Bare Conductive- prints capacitive sense data
 from MPR121 to Serial1 port

 Based on code by Jim Lindblom and plenty of inspiration from the Freescale
 Semiconductor datasheets and application notes.

 Bare Conductive code written by Stefan Dzisiewski-Smith and Peter Krige.


*******************************************************************************/

// Serial1 rate
// for sending debugging via XBee
#define baudRate 57600

#include <MPR121.h>
#include <Wire.h>

void setup() {
  Serial1.begin(baudRate);

  // 0x5C is the MPR121 I2C address on the Bare Touch Board
  if (!MPR121.begin(0x5C)) {
    Serial1.println("error setting up MPR121");
    switch (MPR121.getError()) {
      case NO_ERROR:
        Serial1.println("no error");
        break;
      case ADDRESS_UNKNOWN:
        Serial1.println("incorrect address");
        break;
      case READBACK_FAIL:
        Serial1.println("readback failure");
        break;
      case OVERCURRENT_FLAG:
        Serial1.println("overcurrent on REXT pin");
        break;
      case OUT_OF_RANGE:
        Serial1.println("electrode out of range");
        break;
      case NOT_INITED:
        Serial1.println("not initialised");
        break;
      default:
        Serial1.println("unknown error");
        break;
    }
    while (1);
  }

  // Disable baseline filter
  MPR121.setRegister(NCLR, 0x00); //noise count limit
  MPR121.setRegister(NHDF, 0x01); //noise half delta
  MPR121.setRegister(FDLF, 0x3F); //filter delay limit
}

void loop() {
  readRawInputs();
}

void readRawInputs() {
  // only using electrode 3 on the Touch Board
  int i = 3;

  MPR121.updateAll();

  Serial1.print("FDAT: ");
  //for(i=0; i<13; i++){          // 13 filtered values
  Serial1.print(MPR121.getFilteredData(i), DEC);
  //if(i<12) Serial1.print(" ");
  //}
  Serial1.println();

  Serial1.print("BVAL: ");
  //for(i=0; i<13; i++){          // 13 baseline values
  Serial1.print(MPR121.getBaselineData(i), DEC);
  //if(i<12) Serial1.print(" ");
  //}
  Serial1.println();

  // the trigger and threshold values refer to the difference between
  // the filtered data and the running baseline - see p13 of
  // http://www.freescale.com/files/sensors/doc/data_sheet/MPR121.pdf

  /*Serial1.print("DIFF: ");
  for(i=0; i<13; i++){          // 13 value pairs
    Serial1.print(MPR121.getBaselineData(i)-MPR121.getFilteredData(i), DEC);
    if(i<12) Serial1.print(" ");
  }
  Serial1.println();*/

}
