#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "Adafruit_BLEMIDI.h"
#include "Adafruit_TCS34725.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

#include "BluefruitConfig.h"

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.7.0"
// Color sensor
/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_4X);

// Capacative Touch Includes
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_CAP1188.h>

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

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

Adafruit_BLEMIDI midi(ble);

bool isConnected = false;

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// callback
void connected(void)
{
  isConnected = true;

  Serial.println(F(" CONNECTED!"));
  delay(1000);

}

void disconnected(void)
{
  Serial.println("disconnected");
  isConnected = false;
}

void BleMidiRX(uint16_t timestamp, uint8_t status, uint8_t byte1, uint8_t byte2)
{
  Serial.print("[MIDI ");
  Serial.print(timestamp);
  Serial.print(" ] ");

  Serial.print(status, HEX); Serial.print(" ");
  Serial.print(byte1 , HEX); Serial.print(" ");
  Serial.print(byte2 , HEX); Serial.print(" ");

  Serial.println();
}

void setup() {
  // Serial.begin(9600);

  while (!Serial);  // required for Flora & Micro
  delay(500);

  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit MIDI Example"));
  Serial.println(F("---------------------------------------"));

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

  // Set MIDI RX callback
  midi.setRxCallback(BleMidiRX);

  Serial.println(F("Enable MIDI: "));
  if (!midi.begin(true)) {
    error(F("Could not enable MIDI"));
  }

  ble.verbose(false);
  Serial.print(F("Waiting for a connection..."));

  // Initialize the sensor, if using i2c you can pass in the i2c address
  while (!cap.begin(0x28)) {
    Serial.println("No capacative touch sensor");
    delay(2000);
  }
  Serial.println("CAP1188 (capacative touch sensor) found!");

  // Color sensor
  while (!tcs.begin()) {
    Serial.println("no color sensor");
    delay(2000);
  }
  if (tcs.begin()) {
    Serial.println("Found color sensor");
  } else {
    Serial.println("No TCS34725 (color sensor) found ... check your connections");
    while (1);
  }

}

void loop() {
  // Color sensor
  uint16_t r, g, b, c, colorTemp, lux;
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);
  // Map the R value from the sensor to a value between 8600 (min) and 65535 (max) to 0 (min) and 100 (max)
  int x = map(r, 8600,  65535, 0, 100);
  // Map the B value from the sensor to a value between 14700 (min) and 65535 (max) to 0 (min) and 100 (max)
  int y = map(b, 14700, 65535, 0, 100);
  x = constrain(x, 50, 127);
  y = constrain(y, 50, 127);
  Serial.print("coordiantes:");
  Serial.print(x); Serial.print(y); Serial.println();
  
  // interval for each scanning ~ 500ms (non blocking)
  ble.update(500);

  // bail if not connected
  if (!isConnected) {
    Serial.println("No Bluetooth connection");
    delay(500); 
  }


  // Capacative touch
  uint8_t touched = cap.touched();

  if (touched == 0) {
    // No touch detected
    Serial.println("No touch detected");
  }

  // Determine which section of the capacative touch sensor was touched, if any
  for (uint8_t i=0; i<8; i++) {
    if (touched & (1 << i)) {
      touchedArea = i+1;
      channel = touchedArea;
    }
  }
  pitch = x;
  vel = y;
  // send note on
  sendmidi(channel, 0x9, pitch, vel);
  delay(500);
  // send note off
  sendmidi(channel, 0x8, pitch, 0x0);
  delay(500);
  Serial.print("Sending note on channel");
  Serial.println(channel);

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
