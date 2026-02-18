#line 1 "C:\\Users\\jaanr\\Documents\\GitHub\\EMT0330\\Binary_clock\\Binary_clock.ino"
#include <Arduino.h>
#include <Wire.h>

#include "clock.h"
#include "config.h"
#include "display.h"

namespace {
unsigned long lastLogMillis = 0;  // throttle serial heartbeat
}

#line 12 "C:\\Users\\jaanr\\Documents\\GitHub\\EMT0330\\Binary_clock\\Binary_clock.ino"
void setup();
#line 20 "C:\\Users\\jaanr\\Documents\\GitHub\\EMT0330\\Binary_clock\\Binary_clock.ino"
void loop();
#line 12 "C:\\Users\\jaanr\\Documents\\GitHub\\EMT0330\\Binary_clock\\Binary_clock.ino"
void setup() {
  Serial.begin(115200);      // USB serial for commands and logs
  delay(2000);               // allow serial to open
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);  // custom I2C pins
  displayInit();             // OLED setup
  clockInit();               // start clock at default/start time
}

void loop() {
  clockHandleSerial();                 // process incoming time commands
  ClockTime now = clockCurrent();      // derive current time
  displayUpdate(now);                  // draw to OLED
  if (millis() - lastLogMillis >= 1000UL) {
    Serial.printf("Time: %02u:%02u:%02u\n", now.hour, now.minute, now.second);
    lastLogMillis = millis();
  }
  delay(100);  // modest refresh rate
}

