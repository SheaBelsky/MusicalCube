// ---------------------------------------------------------------------------
//
// ble_neopixel_mpr121.ino
//
// A MIDI sequencer example using two chained NeoPixel sticks, a MPR121
// capacitive touch breakout, and a Bluefruit Feather.
//
// 1x Feather Bluefruit LE 32u4: https://www.adafruit.com/products/2829
// 2x NeoPixel Sticks: https://www.adafruit.com/product/1426
// 1x MPR121 breakout: https://www.adafruit.com/products/1982
//
// Required dependencies:
// Adafruit Bluefruit Library: https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
// Adafruit NeoPixel Library: https://github.com/adafruit/Adafruit_NeoPixel
// Adafruit MPR121 Library: https://github.com/adafruit/Adafruit_MPR121_Library
//
// Author: Todd Treece <todd@uniontownlabs.org>
// Copyright: (c) 2015-2016 Adafruit Industries
// License: GNU GPLv3
//
// ---------------------------------------------------------------------------
#include "FifteenStep.h"
#include "Wire.h"
#include "Adafruit_CAP1188.h"
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BLEMIDI.h"
#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.7.0"

// captouch breakout board init
Adafruit_CAP1188 cap = Adafruit_CAP1188();
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

#define TEMPO    60
#define BUTTONS  4
#define IRQ_PIN  A4

// sequencer init
FifteenStep seq = FifteenStep(1024);
Adafruit_BLEMIDI blemidi(ble);

// prime dynamic values
int channel = 0;
int pitch = 42; 
int vel = 80;
int steps = 16;

bool isConnected = false;

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}
void connected(void)
{
  isConnected = true;
  Serial.println(F(" CONNECTED!"));
}

void disconnected(void)
{
  Serial.println("disconnected");
  isConnected = false;
}

void setup() {

  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }

  //ble.sendCommandCheckOK(F("AT+uartflow=off"));
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();
  
  /* Set BLE callbacks */
  ble.setConnectCallback(connected);
  ble.setDisconnectCallback(disconnected);
  
  Serial.println(F("Enable MIDI: "));
  if ( ! blemidi.begin(true) )
  {
    error(F("Could not enable MIDI"));
  }
    
  ble.verbose(false);
  Serial.print(F("Waiting for a connection..."));
  
  // Initialize the cap touch sensor, if using i2c you can pass in the i2c address
  // if (!cap.begin(0x28)) {
  if (!cap.begin()) {
    Serial.println("CAP1188 not found");
    while (1);
  }
  Serial.println("CAP1188 found!");

  // start sequencer and set callbacks
  seq.begin(TEMPO, steps);
  seq.setMidiHandler(midi);
//  seq.setStepHandler(step);

}

void loop() {

  uint8_t touched = cap.touched();

  if (touched == 0) {
    // No touch detected
    return;
  }
  
  for (uint8_t i=0; i<4; i++) {
    if (touched & (1 << i)) {
      Serial.print("C"); Serial.print(i+1); Serial.print("\t");
      // set midi channel to the selected touch pad, 1 to 4
      channel = i+1;
      handle_note();
    }
  }
  Serial.println();
  delay(100);

  // this is needed to keep the sequencer
  // running. there are other methods for
  // start, stop, and pausing the steps
  seq.run();

}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                         SEQUENCER/MIDI CALLBACKS                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// the callback that will be called by the sequencer when it needs
// to send midi commands over BLE.
void midi(byte channel, byte command, byte arg1, byte arg2) {

  // init combined byte
  byte combined = command;

  // shift if necessary and add MIDI channel
  if(combined < 128) {
    combined <<= 4;
    combined |= channel;
  }

  blemidi.send(combined, arg1, arg2);

}

// deal with note on and off
void handle_note() {
  // play pressed note
  midi(channel, 0x9, pitch, vel);
  // play note off
  // midi(channel, 0x8, pitch[i], 0x0);
}
