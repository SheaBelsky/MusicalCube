#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "Adafruit_BLEMIDI.h"
#include <Adafruit_CAP1188.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_TCS34725.h"
#include <SPI.h>
#include <Wire.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

#include "BluefruitConfig.h"

// Definitions
#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.7.0"
#define RGB_PIN A5
#define VBATPIN A9

// RGB Ring
int numPins = 24;
int defaultWait = defaultWait;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPins, RGB_PIN, NEO_GRB + NEO_KHZ800);
// Maintains state of ring color
uint32_t ringColor;

// Color sensor
/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_4X);

// Capacative Touch Includes

// captouch breakout board init
Adafruit_CAP1188 cap = Adafruit_CAP1188();

#define BUTTONS  4

// prime dynamic values
int channel = 1;
int pitch = 80;
int vel = 80;

// Variable to maintain the state of the last touched area
int touchedArea;

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_BLEMIDI midi(ble);

bool isConnected = false;

// A small helper
void error(const __FlashStringHelper*err) {
  if (true) {
    Serial.println(err);
  }
  while (1);
}

// callback
void connected(void) {
  isConnected = true;

  Serial.println(F(" CONNECTED!"));
  delay(1000);

}

void disconnected(void) {
  Serial.println("disconnected");
  isConnected = false;
}

void BleMidiRX(uint16_t timestamp, uint8_t status, uint8_t byte1, uint8_t byte2) {
  if (true) {
    Serial.print("[MIDI ");
    Serial.print(timestamp);
    Serial.print(" ] ");
  
    Serial.print(status, HEX); Serial.print(" ");
    Serial.print(byte1 , HEX); Serial.print(" ");
    Serial.print(byte2 , HEX); Serial.print(" ");
  
    Serial.println();
  }
}

void setup() {
  while (!Serial);  // required for Flora & Micro
  delay(500);

  Serial.begin(115200);
  Serial.print("Checking for Bluetooth...");
  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) ){
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

  Serial.println("Bluetooth good!");
  Serial.println("==============");
  Serial.println("Checking MIDI connection...");

  // Set MIDI RX callback
  midi.setRxCallback(BleMidiRX);

  Serial.println(F("Enable MIDI: "));
  while (!midi.begin(true)) {
    Serial.println("Could not enable MIDI");
    delay(2000);
  }
  ble.verbose(false);
  Serial.println("MIDI good!");
  Serial.println("==============");
  Serial.println("Checking capacative touch sensor...");

  // Initialize the sensor, if using i2c you can pass in the i2c address
  while (!cap.begin(0x28)) {
    Serial.println("No capacative touch sensor");
    delay(2000);
  }
  Serial.println("CAP1188 (capacative touch sensor) found!");
  Serial.println("==============");
  Serial.println("Checking RGB color sensor...");
  
  // Color sensor
  while (!tcs.begin()) {
    Serial.println("No RGB color sensor found");
    delay(2000);
  }
  Serial.println("TCS34725 (RGB color sensor) found!");
  Serial.println("==============");
  Serial.println("Checking RGB ring...");
  
  // Begin the RGB strip
  strip.begin();
  strip.setBrightness(20);
  // Set all the pixels to be off
  strip.show();
  Serial.println("RGB ring configured!");
  Serial.println("==============");
  Serial.println("All systems ready for takeoff");
}

void loop() {
  if (true) {
    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    Serial.print("VBat: " ); Serial.println(measuredvbat);
  }
  
  // Color sensor
  uint16_t r, g, b, c, colorTemp, lux;
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);
  
  Serial.print("red: "); Serial.print(r); Serial.println();
  Serial.print("blue: "); Serial.print(b); Serial.println();
  
  // Map the R value from the sensor to a value between 8600 (min) and 65535 (max) to 0 (min) and 100 (max)
  // pitch
  int x = map(r, 4620,  65535, 40, 127);
  // Map the B value from the sensor to a value between 14700 (min) and 65535 (max) to 0 (min) and 100 (max)
  // volume
  int y = map(b, 3725, 53030, 60, 127);
  if (true) {
    Serial.print("Coordinates: ");
    Serial.print(x); Serial.print(", "); Serial.print(y); Serial.println();
  }
  
  
  // interval for each scanning ~ 500ms (non blocking)
  ble.update(10);

  // bail if not connected
  if (true) {
    Serial.println("No Bluetooth connection");
    //delay(500);
  }


  // Capacative touch
  uint8_t touched = cap.touched();

  if (touched == 0 && true) {
    // No touch detected
    Serial.println("No touch detected");
  }

  // Determine which section of the capacative touch sensor was touched, if any
  for (uint8_t i=0; i<8; i++) {
    if (touched & (1 << i)) {
      touchedArea = i+1;
      channel = touchedArea + 4;
    }
  }
  pitch = x;
  vel = y;
  // send note on
  sendmidi(channel, 0x9, pitch, vel);
  delay(200);
  // send note off
  sendmidi(channel, 0x8, pitch, 0x0);
  
  if (true) {
    Serial.print("Sending note on channel ");
    Serial.println(channel);
  }
  Serial.print("touched area"); Serial.print(touchedArea); Serial.println();
  

  // RGB ring update
  switch(touchedArea) {
    case 1:
      ringColor = strip.Color(255, 0, 0);
      break;
    case 2:
      ringColor = strip.Color(0, 255, 0);
      break;
    case 3:
      ringColor = strip.Color(0, 0, 255);
      break;
    case 4:
      ringColor = strip.Color(255, 255, 0);
      break;
    default:
      break;
  }
  colorWipe(ringColor, defaultWait);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

// the callback that will be called to send midi commands over BLE.
// example usage - note on
// sendmidi(channel, 0x9, pitch, vel);
// example usage - note off
// sendmidi(channel, 0x8, pitch, 0x0);
void sendmidi(byte channel, byte command, byte arg1, byte arg2) {

  // init combined byte
  byte combined = command;

  // shift if necessary and add MIDI channel
  if(combined < 128) {
    combined <<= 4;
    combined |= channel;
  }

  midi.send(combined, arg1, arg2);
}
