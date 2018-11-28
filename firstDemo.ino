#include <Wire.h>
#include "Adafruit_TCS34725.h"
/* Example code for the Adafruit TCS34725 breakout library */

/* Connect SCL    to analog 5
   Connect SDA    to analog 4
   Connect VDD    to 3.3V DC
   Connect GROUND to common ground */
   
/* Initialise with default values (int time = 2.4ms, gain = 1x) */
// Adafruit_TCS34725 tcs = Adafruit_TCS34725();

/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_4X);

void setup(void) {
  Serial.begin(9600);
  while (!tcs.begin()) {
    Serial.println("no sensor");
    delay(2000);
  }
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }
}

void loop(void) {
  
  while(1) {
    
  uint16_t r, g, b, c, colorTemp, lux;
  
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);

  // Red corner - 65535 for red value, 26000 for blue value
  // Blue corner - 17000 for red value, 65535 for blue value
  // White corner - 65535 for red value, 65535 for blue value
  // Black corner - 12000 for red value, 17000 for blue value

  // Map the R value from the sensor to a value between 8600 (min) and 65535 (max) to 0 (min) and 100 (max)
  int x = map(r, 8600,  65535, 0, 100);
  // Map the R value from the sensor to a value between 14700 (min) and 65535 (max) to 0 (min) and 100 (max)
  int y = map(b, 14700, 65535, 0, 100);
  x = constrain(x, 0, 100);
  y = constrain(y, 0, 100);
//  Serial.print("Color Temp: "); Serial.print(colorTemp, DEC); Serial.print(" K - ");
//  Serial.print("Lux: "); Serial.print(lux, DEC); Serial.print(" - ");
//  Serial.print("R: "); Serial.print(r); Serial.print(" ");
//  Serial.print("G: "); Serial.print(g); Serial.print(" ");
//  Serial.print("B: "); Serial.print(b); Serial.print(" ");
//  Serial.print("C: "); Serial.print(c, DEC); Serial.print(" ");
//  Serial.print("X: "); Serial.print(x); Serial.print(" ");
//  Serial.print("Y: "); Serial.print(y); Serial.print(" ");
//  Serial.println(" ");

  // send data to Processing
  String message;
  message += x;
  message += ",";
  message += y;
  message += ".";
  Serial.print(message);
  while(Serial.available() == 0 || Serial.read() != 'N');
  
  // delay(1000);
  }
}
