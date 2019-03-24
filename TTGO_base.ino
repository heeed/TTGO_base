/**************************************************************************
 Based on the Adafruit_SSD1306 example: ssd1306_128x64_i2c
 This is for the TTGO ESP32 OLED module
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HomeWifi.h>             // my lib with my wifi credentials

/* THIS IS INCOMPLETE CODE */

#define LOOP_INTERVAL 600         // milliseconds between each cycle through main loop
#define MSG_PERSIST_CYCLES 100    // for how many cycles through the loop should messages persist?
#define ERR_PERSIST_CYCLES 100    // same for error messages
#define SENSOR_INTERVAL 100       // how many cycles between sensor readings & temp updating
#define TIME_INTERVAL 20          // how often is time display updated
#define DATE_INTERVAL 1000        // how often is date display updated

// --- WIFI -------------------------------------------------------------------------------
#define SERVER_ERR_LIMIT 5
#define WLAN_PASS HOME_WIFI_PW // from HomeWifi.h
#define WIFI_MAX_TRIES 12
const char* ssid [] = { HOME_WIFI_AP_MAIN, HOME_WIFI_AP_ALT }; // I have more than one AP at home
int wifi_status = WL_IDLE_STATUS;
IPAddress ip;
uint8_t server_errors = 0;

// --- OLED DISPLAY -----------------------------------------------------------------------
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     16 // Reset pin - changed to 16 for TTGO board
#define TEMP_X_POS 1
#define TEMP_Y_POS 36
TwoWire twi = TwoWire(1); // create our own TwoWire instance
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &twi, OLED_RESET); // passing our TwoWire instance as param

// --- INFO ------------------------------------------------------------------------------
String timeStr = "--:--";
String dateStr = "-";
String weatherStr = "";

// ---------------------------------------------------------------------------------------
// --- GLOBALS                                                                         ---
// ---------------------------------------------------------------------------------------
int msg_counter, err_counter = 0;
int sensor_counter = SENSOR_INTERVAL; // to force an immediate read
int time_counter = TIME_INTERVAL - 1;
int date_counter = DATE_INTERVAL - 1;
int weather_counter = WEATHER_INTERVAL - 1;

// ---------------------------------------------------------------------------------------
// --- WIFI FUNCTIONS                                                                  ---
// ---------------------------------------------------------------------------------------
void wifiConnect() {
  uint8_t ssid_idx = 0;
  uint8_t connect_counter = 0;
  while (connect_counter < WIFI_MAX_TRIES) {
    Serial.print("Attempting to connect to "); Serial.println(ssid[ssid_idx]);
    WiFi.begin(ssid[ssid_idx], WLAN_PASS);  // try to connect
    // delay to allow time for connection
    for (uint8_t i = 0; i < 10; i++) {
      //digitalWrite(WIFI_CONNECT_LED, HIGH);
      delay(250);
      //digitalWrite(WIFI_CONNECT_LED, LOW);
      delay(250);
    }
    wifi_status = WiFi.status();
    connect_counter++;
    if (wifi_status != WL_CONNECTED) {
      ssid_idx = 1 - ssid_idx;    // swap APs
    } else {
      Serial.println("Connected!");
      //digitalWrite(WIFI_CONNECT_LED, HIGH);
      connect_counter = WIFI_MAX_TRIES; // to break out of the loop
      ip = WiFi.localIP();
      Serial.print("IP: "); Serial.println(ip);
      server_errors = 0;
    }
  }
  if (wifi_status != WL_CONNECTED) {  // wifi connection failed
    server_errors = SERVER_ERR_LIMIT;
  }
}

// ---------------------------------------------------------------------------------------
// --- DISPLAY FUNCTIONS                                                               ---
// ---------------------------------------------------------------------------------------
String dataString(DataFormat format) {
  char temp_buf[5];
  char minmax_buf[6];
  sprintf(minmax_buf, "%i/%i", temp_min, temp_max);
  dtostrf(temp_float, 4, 1, temp_buf);
  String sensor_vals = "";
  if (format == DATA_FMT_SCREEN) {
    sensor_vals = String(temp_buf) + "  " + String(minmax_buf);
  } else if(format == DATA_FMT_REPORT) {
    sensor_vals = String(temp_buf) + "_null_" + String(minmax_buf);
  }
  return sensor_vals;
}

void printTemperature(int8_t oldtemp, int8_t newtemp) {
  char buf[3];
  sprintf(buf, "%i", oldtemp);           // put current temp in buffer
  display.setFont(&FreeSans24pt7b);
  // erase existing temperature
  int16_t  x1, y1;
  uint16_t w, h;
  display.getTextBounds(buf, TEMP_X_POS, TEMP_Y_POS, &x1, &y1, &w, &h);
  display.fillRect(x1, y1, w, h, BLACK);
  display.setCursor(TEMP_X_POS,TEMP_Y_POS); 
  sprintf(buf, "%i", newtemp);
  display.print(buf);
  display.display();
}

void printDegC() {
  display.setFont(&FreeSans9pt7b);
  display.setCursor(52,13);
  display.print("o");
  display.setFont(&FreeSans12pt7b);
  display.setCursor(62,22);
  display.print("C");
}

void printIP() {
  display.setFont();           // stop using GFX fonts
  display.setTextSize(1);      // printable sizes from 1 to 8; typical use is 1, 2 or 4
  display.setCursor(1,56);
  display.print(ip.toString());
}

void printTime() {
  display.setFont(&FreeSans9pt7b);
  display.fillRect(66, 26, 63, 20, BLACK);
  display.setCursor(66,42);
  display.print(timeStr);
  display.display();
}

void printDate() {
  display.setFont(&FreeSans9pt7b);
  display.fillRect(66, 44, 63, 20, BLACK);
  display.setCursor(66,62);
  display.print(dateStr);
  display.display();  
}

// ---------------------------------------------------------------------------------------
// --- SENSOR FUNCTIONS                                                                ---
// ---------------------------------------------------------------------------------------

void updateTemperature() {
  temp_prev = temp_int;
  temp_float = random(12,30);
  //temp_float = dht.readTemperature() - 0.5;
  temp_int = round(temp_float);
  if(temp_int > temp_max) temp_max = temp_int;
  if(temp_int < temp_min) temp_min = temp_int;
}

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

  wifiConnect();

  // Clear the buffer
  display.clearDisplay();
  display.setTextColor(WHITE); // or BLACK);
  printDegC();
  printIP();
  display.display();

}

// ***************************************************************************************
// ***  LOOP                                                                           ***
// ***************************************************************************************

void loop() {

  if (sensor_counter == SENSOR_INTERVAL) {
    // if(now - lastSensorRead >= SENSOR_INTERVAL) { .. }
    sensor_counter = 0;
    updateTemperature();
    printTemperature(temp_prev, temp_int);
  } 

  if (date_counter == DATE_INTERVAL) {
    date_counter = 0;
    getDateTime(DATE_INFO);
    printDate();
  }

  if (time_counter == TIME_INTERVAL) {
    time_counter = 0;
    getDateTime(TIME_INFO);
    printTime();
  }

 sensor_counter++; time_counter++; date_counter++;
 
  if (server_errors >= SERVER_ERR_LIMIT) {
    timeStr = "--:--";
    dateStr = ".";
    wifiConnect();
  }
  
  delay(LOOP_INTERVAL);
}
