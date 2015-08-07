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
#define millisInTenMinutes 300000

//#define millisin20Seconds
#define numSounds 15

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
unsigned long timeSinceLastStop;

int sensorButt = 11;
int sensorTouch = 0;
int soundIndex = 1;

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
  Serial.begin(baudRate);

  // 0x5C is the MPR121 I2C address on the Bare Touch Board
  if (!MPR121.begin(0x5C)) {
    Serial.println("error setting up MPR121");
    switch (MPR121.getError()) {
      case NO_ERROR:
        Serial.println("no error");
        break;
      case ADDRESS_UNKNOWN:
        Serial.println("incorrect address");
        break;
      case READBACK_FAIL:
        Serial.println("readback failure");
        break;
      case OVERCURRENT_FLAG:
        Serial.println("overcurrent on REXT pin");
        break;
      case OUT_OF_RANGE:
        Serial.println("electrode out of range");
        break;
      case NOT_INITED:
        Serial.println("not initialised");
        break;
      default:
        Serial.println("unknown error");
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
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
  }

  // initialise array
  for (int i = 0; i < sizeLookBack; i++ ) {
    lastValues[i] = 0;
  }
  //set thresholds for butt sensor
  MPR121.setTouchThreshold(sensorButt, 20);
  MPR121.setReleaseThreshold(sensorButt, 10);
  timeSinceLastStop = millis();
}

void loop() {
  readRawInputs();
}

void readRawInputs() {
  
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
    soundIndex+=2;
    if(soundIndex > numSounds) soundIndex = 1;
    Serial.print(soundIndex);
    Serial.println();
  }
  if( MPR121.isNewRelease(sensorButt) ) {
    buttTriggered = false;
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

// check butt sensor
  if( MPR121.isNewTouch(sensorButt) ) {
    soundIndex += 2;
    if(soundIndex > 13) soundIndex = 1;
    buttTriggered = true;
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if( MPR121.isNewRelease(sensorButt) ) {
    buttTriggered = false;
    digitalWrite(LED_BUILTIN, LOW);
  };
  
  //only activate noises if butt sensor is touched
  if(buttTriggered) {
    // if seat has been triggered but track not playing yet, play it
    if (seatTriggered && !touchTriggered ) {
      if ( !seatPlaying ) {
        // if playing audio, stop it
        if (MP3player.isPlaying()) {
          MP3player.stopTrack();
          Serial.println("stopped 0");
        }
        // play the touch track
        MP3player.playTrack(soundIndex);
        timeSinceLastStop = millis();
        seatPlaying = true;
        touchPlaying = false;
      } else{
        MP3player.playTrack(soundIndex);
      }
    }
    if (touchTriggered) {
      if ( !touchPlaying ) {
        // if playing seat audio, stop it
        if (MP3player.isPlaying()) {
          MP3player.stopTrack();
          Serial.println("stopped 1");
        }
        // play the touch track
        MP3player.playTrack(soundIndex + 1);
        timeSinceLastStop = millis();
        seatPlaying = false;
        touchPlaying = true;
      } else {
        MP3player.playTrack(soundIndex + 1);
      }
    }
  } else if (seatPlaying || touchPlaying){
    //Stop track if butt sensor is not active
    MP3player.stopTrack();
    Serial.println("stopped 2");
    seatPlaying = false;
    touchPlaying = false;
  }
  if ( !seatTriggered && !touchTriggered && (seatPlaying || touchPlaying)) {
    // if not within threshold, stop any playing
    MP3player.stopTrack();
    Serial.println("stopped 3");
    seatPlaying = false;
    touchPlaying = false;
  }
  
  if(millis() - timeSinceLastStop > millisInTenMinutes) {
    MP3player.playTrack(0);
    //timeSinceLastStop = millis();
    Serial.print("keep alive played");
    Serial.println();
  }
  Serial.print("buttOn: ");
  Serial.print(buttTriggered);
  Serial.println();
  
}

void updateLastValues(int latest) {
  // shift current values in array to next address
  for ( int i = 1; i < sizeLookBack; i++) {
    lastValues[i] =  lastValues[i - 1];
  }
  // make latest value first item in array
  lastValues[0] = latest;
}
