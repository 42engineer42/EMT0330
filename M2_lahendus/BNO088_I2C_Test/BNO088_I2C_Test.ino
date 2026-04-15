#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include <sh2_SensorValue.h>

constexpr uint8_t BNO08X_SDA_PIN = 42;
constexpr uint8_t BNO08X_SCL_PIN = 41;
constexpr uint8_t BNO08X_ADDR = 0x4A;
constexpr uint32_t ACCEL_REPORT_INTERVAL_US = 50000;

Adafruit_BNO08x bno08x(-1);
sh2_SensorValue_t sensorValue;

static void enableReports() {
  if (!bno08x.enableReport(SH2_ACCELEROMETER, ACCEL_REPORT_INTERVAL_US)) {
    Serial.println(F("Failed to enable accelerometer report"));
  }
}

static void printEvent(const sh2_SensorValue_t &event) {
  if (event.sensorId == SH2_ACCELEROMETER) {
    Serial.print(F("\rACC m/s^2  X: "));
    Serial.print(event.un.accelerometer.x, 3);
    Serial.print(F(" Y: "));
    Serial.print(event.un.accelerometer.y, 3);
    Serial.print(F(" Z: "));
    Serial.print(event.un.accelerometer.z, 3);
    Serial.print(F("    "));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("BNO088 I2C test starting"));
  Serial.print(F("Using SDA pin "));
  Serial.print(BNO08X_SDA_PIN);
  Serial.print(F(", SCL pin "));
  Serial.println(BNO08X_SCL_PIN);

  Wire.begin(BNO08X_SDA_PIN, BNO08X_SCL_PIN);
  Wire.setClock(400000);

  if (!bno08x.begin_I2C(BNO08X_ADDR, &Wire)) {
    Serial.println(F("BNO088 not detected at 0x4A"));
    Serial.println(F("Check wiring, power, and address pin state"));
    while (true) {
      delay(100);
    }
  }

  Serial.println(F("BNO088 detected"));
  Serial.println(F("Printing accelerometer only"));
  enableReports();
}

void loop() {
  if (bno08x.wasReset()) {
    Serial.println(F("Sensor reset detected, re-enabling reports"));
    enableReports();
  }

  if (bno08x.getSensorEvent(&sensorValue)) {
    printEvent(sensorValue);
  }
}
