#include <math.h>

void setup() {
  Serial.begin(9600);
}

void loop() {
  float rgb[3] = {0.0,0.0,0.0};
  kelvinToRgb(5000, rgb);
  Serial.println(rgb[0]);
  Serial.println(rgb[1]);
  Serial.println(rgb[2]);
  Serial.println("=====");
  delay(2000);
}

float kelvinToRgb(float kelvin, float *a) {
  float temp = kelvin / 100.0;
  float red = 0;
  float green = 0;
  float blue = 0;
  if (temp < 66.0) {
    red = 255.0;
    green = temp;
    green = 99.4708025861 * log(green) - 161.1195681661;
    if (temp <= 19){
        blue = 0;
    }
    else {
        blue = temp-10;
        blue = 138.5177312231 * log(blue) - 305.0447927307;
    }
  }
  else {
      red = temp - 60;
      red = 329.698727446 * pow(red, -0.1332047592);

      green = temp - 60;
      green = 288.1221695283 * pow(green, -0.0755148492 );

      blue = 255;
  }
  a[0] = red;
  a[1] = green;
  a[2] = blue;
  return 0;
}
