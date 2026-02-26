#include <Arduino.h>
#include <Wire.h>

#include "clock.h"
#include "config.h"
#include "display.h"
#include "xbox.h"

namespace {
unsigned long lastLogMillis = 0;  // throttle serial heartbeat
}

void setup() {
  Serial.begin(115200);      // USB serial for commands and logs
  delay(2000);               // allow serial to open
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);  // custom I2C pins
  displayInit();             // OLED setup
  clockInit();               // start clock at default/start time
  xboxInit();                // enable gamepad control
}

void loop() {
  clockHandleSerial();                 // process incoming time commands
  xboxUpdate();                        // poll controller
  ClockTime now = clockCurrent();      // derive current time
  displayUpdate(now);                  // draw to OLED
  if (millis() - lastLogMillis >= 1000UL) {
    Serial.printf("Time: %02u:%02u:%02u\n", now.hour, now.minute, now.second);
    lastLogMillis = millis();
  }
  delay(100);  // modest refresh rate
}
