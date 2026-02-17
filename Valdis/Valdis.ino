#include <Arduino.h>
#include "xbox.h"
#include "Constants.h"

namespace {
constexpr uint32_t kPrintIntervalMs = 100;
uint32_t lastPrintMs = 0;
}

void setup() {
  if (SERIAL_PRINT_ENABLED) {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting Xbox throttle monitor...");
  }

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);

  if (SERIAL_PRINT_ENABLED) {
    Serial.println("Ready. Move triggers/stick to see left/right motor throttle.");
  }
}

void loop() {
  const bool dataUpdated = BP32.update();
  if (!dataUpdated) {
    delay(10);
    return;
  }

  processConnectedControllers();

  const uint32_t now = millis();
  if (SERIAL_PRINT_ENABLED && (now - lastPrintMs >= kPrintIntervalMs)) {
    lastPrintMs = now;
    printDriveDebug();
  }

  //delay(5);
}
