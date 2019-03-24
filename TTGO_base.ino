/**************************************************************************
 Based on the Adafruit_SSD1306 example: ssd1306_128x64_i2c
 This is for the TTGO ESP32 OLED module
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

// --- OLED DISPLAY -----------------------------------------------------------------------
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     16 // Reset pin - changed to 16 for TTGO board
TwoWire twi = TwoWire(1); // create our own TwoWire instance
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &twi, OLED_RESET); // passing our TwoWire instance as param

// ***************************************************************************************
// ***  SETUP                                                                          ***
// ***************************************************************************************
void setup() {
  Serial.begin(115200);
  twi.begin(4,15);  // Needs to come before display.begin() is used
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
}

// ***************************************************************************************
// ***  LOOP                                                                           ***
// ***************************************************************************************

void loop() {
}
