/*******************************************************************************

 Chair Sensor
 ------------------------------
 Detects a user sitting in a chair and then plays a sound file continuously
 until they leave. If they reach out and touch a large capacitive object,
 like another person, the sound changes.

 Written by Becky Stewart, 2015

 Adapted from DataStream.ino by Bare Conductive- prints capacitive sense data
 from MPR121 to Serial1 port; and Touch_MP3 by Bare Conductive

 Based on code by Jim Lindblom and plenty of inspiration from the Freescale
 Semiconductor datasheets and application notes.

 Bare Conductive code written by Stefan Dzisiewski-Smith and Peter Krige.


*******************************************************************************/

// Serial1 rate
// for sending debugging via XBee
#define baudRate 57600

#include <MPR121.h>
#include <Wire.h>

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <SFEMP3Shield.h>

// mp3 variables
SFEMP3Shield MP3player;
byte result;
boolean seatPlaying = false;
boolean touchPlaying = false;
boolean buttPlaying = false;



// sitting and touch variables
int seatThreshold = 100;
int touchThreshold = 8;
int buttThreshold = 8;
int sizeLookBack = 30;
int lastValues[30];

boolean seatTriggered = false;
boolean touchTriggered = false;
boolean buttTriggered = false;
int seatValue = 300;
long seatTime = 0;

// sd card instantiation
SdFat sd;

void setup() {
  Serial1.begin(baudRate);
  Serial.begin(baudRate);

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

  // set up audio playback
  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();

  result = MP3player.begin();
  MP3player.setVolume(10, 10);

  if (result != 0) {
    Serial1.print("Error code: ");
    Serial1.print(result);
    Serial1.println(" when trying to start MP3 player");
  }

  // initialise array
  for (int i = 0; i < sizeLookBack; i++ ) {
    lastValues[i] = 0;
  }
  //set thresholds for butt sensor
  MPR121.setTouchThreshold(4, 8);
  MPR121.setReleaseThreshold(4, 3);
  
}

void loop() {
  readRawInputs();
}

void readRawInputs() {
  // We want to use electrode 3 to sense for touches
  int sensorTouch = 3;
  
  // We want to use electrode 4 to sense for butts
  int sensorButt = 4;
  
  // Update the touch data
  MPR121.updateAll();

  // log some data
//  Serial.print("FDAT: ");
//  Serial.print(MPR121.getFilteredData(sensorTouch), DEC);
//  Serial.println();
//
//  Serial.print("BVAL: ");
//  Serial.print(MPR121.getBaselineData(sensorTouch), DEC);
//  Serial.println();
  
  if( MPR121.isNewTouch(sensorButt) ) {
    buttTriggered = true;
    Serial.println("Butt triggered");
  }
  if( MPR121.isNewRelease(sensorButt) ) {
    buttTriggered = false;
    Serial.println("Butt released");
  };
  
  // Determine whether there is a touch on the contact and if that person is touching another person
  // update array of past values with current reading
  updateLastValues(MPR121.getBaselineData(sensorTouch) - MPR121.getFilteredData(sensorTouch));

  // go through current array of values
  boolean tempSeatTriggered = true;
  int tempSum = 0;

  for (int i = 0; i < sizeLookBack; i++) {
    // if any of the values are below the threshold, don't trigger sitting
    if ( lastValues[i] < seatThreshold ) {
      tempSeatTriggered = false;
    }

    tempSum = tempSum + lastValues[i]; // running total
  }
  int avgValue = tempSum / sizeLookBack;

  // calculate the standard deviation
  int stanDev[sizeLookBack];
  // initialise array
  for (int i = 0; i < sizeLookBack; i++ ) {
    stanDev[i] = 0;
  }

  boolean steadyStan = true;
  for ( int i = 0; i < sizeLookBack; i++ ) {
    stanDev[i] = avgValue - lastValues[i];
    stanDev[i] = stanDev[i] * stanDev[i];
    if ( stanDev[i] > 2 ) { // adjust this number to change sensitivity
      steadyStan = false;
    }

    //Serial.print("stan dev: ");
    //Serial.println(stanDev[i]);
  }
  //Serial.println("-----");

  // if above the seat threshold
  // and if standard deviation is 0
  if ( tempSeatTriggered && steadyStan && !seatTriggered) {
    // set seat sound to trigger
    seatTriggered = true;
    // start debounce timer to let seatValue settle
    seatTime = millis();

  } if ( !tempSeatTriggered) {
    seatTriggered = false;
  }
  // enough time has passed, set seatValue
  if (seatTriggered && (millis() - seatTime > 600) && (millis() - seatTime < 650)) {
    // store the current value as the seat value
    seatValue = avgValue;
  }

  // if seat sound is triggered and now also above touch threshold
  // and now no longer a steady state, trigger touch sound
  if ( seatTriggered && avgValue > (seatValue + touchThreshold) ) {
    touchTriggered = true;
  } else {
    touchTriggered = false;
  }

//  Serial.print("avg value: ");
//  Serial.println(avgValue);
//  Serial.print("seat value: ");
//  Serial.println(seatValue);
//  Serial.print("SEAT: ");
//  Serial.println(seatTriggered);
//  Serial.print("TOUCH: ");
//  Serial.println(touchTriggered);
//  Serial.println("================");

// check butt sensor
  if( MPR121.isNewTouch(sensorButt) ) {
    buttTriggered = true;
  }
  if( MPR121.isNewRelease(sensorButt) ) {
    buttTriggered = false;
  };
  
  // Determine Which track is to be played
  // if seat has been triggered but track not playing yet, play it
  if(buttTriggered) {
    if(seatTriggered) {
      if(touchTriggered) {
        // seat and touch are triggered
        if(!touchPlaying) {
          if(MP3player.isPlaying()) {
            MP3player.stopTrack();
          }
          MP3player.playTrack(2);
          buttPlaying = false;
          seatPlaying = false;
          touchPlaying = true;
        } else {
          MP3player.playTrack(2);
        }
      } else {
        // just seat is triggered
        if(!seatPlaying) {
          if(MP3player.isPlaying()) {
            MP3player.stopTrack();
          }
          MP3player.playTrack(1);
          buttPlaying = false;
          seatPlaying = true;
          touchPlaying = false;
        } else {
          MP3player.playTrack(1);
        }
      }
    } else {
      // just the butt is triggered
        if(!buttPlaying) {
          if(MP3player.isPlaying()) {
          MP3player.stopTrack();
        }
        MP3player.playTrack(0);
        buttPlaying = true;
        seatPlaying = false;
        touchPlaying = false;
      } else {
        MP3player.playTrack(0);
      }
    }
  } else {
    MP3player.stopTrack();
  }
}

void updateLastValues(int latest) {
  // shift current values in array to next address
  for ( int i = 1; i < sizeLookBack; i++) {
    lastValues[i] =  lastValues[i - 1];
  }
  // make latest value first item in array
  lastValues[0] = latest;
}
