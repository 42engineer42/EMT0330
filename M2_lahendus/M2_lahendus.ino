#include "Config.h"
#include "MotorControl.h"
#include "LEDStrip.h"
#include <Wire.h>

// --------------------
// Mootorite kiirused (0..255)
// --------------------
const uint8_t FAST_SPEED = 180;
const uint8_t SLOW_SPEED = 30;
const unsigned long I2C_SCAN_INTERVAL_MS = 3000;

// Testi oleku muutujad
uint8_t driveState = 0;
unsigned long lastDriveStep = 0;
unsigned long lastI2CScan = 0;

// --------------------
// LED-riba: mitu tuld ja mis pinni kulge on uhendatud LEDid voetakse Config.h-st
// --------------------
LEDStrip leds(PIN_WS2812, NUM_PIXELS);

void scanI2CBus() {
  uint8_t deviceCount = 0;

  Serial.println();
  Serial.println(F("I2C scan started"));

  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print(F("Found device at 0x"));
      if (address < 16) {
        Serial.print('0');
      }
      Serial.println(address, HEX);
      ++deviceCount;
    } else if (error == 4) {
      Serial.print(F("Unknown error at 0x"));
      if (address < 16) {
        Serial.print('0');
      }
      Serial.println(address, HEX);
    }
  }

  if (deviceCount == 0) {
    Serial.println(F("No I2C devices detected"));
  } else {
    Serial.print(F("I2C scan complete, devices found: "));
    Serial.println(deviceCount);
  }
}

// -------------------------------------------------------------
// Valikuline 5-sekundiline ootamine alguses
// Kasuta loopis: runStartupDelayOnce();
// -------------------------------------------------------------
void runStartupDelayOnce() {
    static bool alreadyWaited = false;

    if (!alreadyWaited) {
        alreadyWaited = true;
        delay(5000);
    }
}

// -------------------------------------------------------------
// Lihtne mootorite TEST
// NB! See on ainult katsetamiseks, mitte paris soiduloogika.
// -------------------------------------------------------------
void motorTest() {
  Serial.println(F("Test: Forward"));
  MotorControl::moveForward(FAST_SPEED);
  delay(2000);

  Serial.println(F("Stop"));
  MotorControl::stop();
  delay(1000);

  Serial.println(F("Test: Backward"));
  MotorControl::moveBackward(FAST_SPEED);
  delay(2000);

  Serial.println(F("Stop"));
  MotorControl::stop();
  delay(1000);

  Serial.println(F("Test: Turn Left"));
  MotorControl::turnLeft(FAST_SPEED, SLOW_SPEED);
  delay(2000);

  Serial.println(F("Stop"));
  MotorControl::stop();
  delay(1000);

  Serial.println(F("Test: Turn Right"));
  MotorControl::turnRight(SLOW_SPEED, FAST_SPEED);
  delay(2000);

  Serial.println(F("Stop"));
  MotorControl::stop();
  delay(2000);
}

// -------------------------------------------------------------
// SIIN ON SOIDULOGIKA FUNKTSIOON (PRAEGU TUHI).
// KUTSU SEDA loop() SEES: driveLogic();
// -------------------------------------------------------------
void driveLogic() {
  /*
  OLULISED SOOVITUSED:
  - Parast luhikest pooramist voi muud liikumist kasuta korra MotorControl::stop().
  - Ara pane liiga pikki delay() vaartusi, muidu robot reageerib aeglaselt.
  */
}

// -------------------------------------------------------------
// setup() - kaivitub uks kord roboti sisselulitamisel
// -------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);

    Serial.println(F("Robot startup"));
    Serial.print(F("I2C SDA pin: "));
    Serial.println(I2C_SDA);
    Serial.print(F("I2C SCL pin: "));
    Serial.println(I2C_SCL);
    scanI2CBus();

    MotorControl::begin();
    leds.begin();
    leds.clear();
    leds.show();
}

// -------------------------------------------------------------
// loop() - jookseb kogu aeg uuesti ja uuesti
// -------------------------------------------------------------
void loop() {
  // runStartupDelayOnce(); // eemalda kommentaar kui tahad 5 sek alguspausi

  // -------------------------------------- TESTID ---------------------------------------------------------------//
  // TEST 1
  // motorTest();

  // TEST 2
  // MotorControl::motorTestUpdate();

  if (millis() - lastI2CScan >= I2C_SCAN_INTERVAL_MS) {
    lastI2CScan = millis();
    scanI2CBus();
  }

  // SIIN KUTSU OMA SOIDULOGIKA
  // driveLogic();
}
