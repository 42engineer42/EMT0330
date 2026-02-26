#include "xbox.h"

#include "clock.h"

#include <cstdio>
#include <cstring>

// Pairing target: labipaistev Sinine
#define CONTROLLER_MAC "40:8e:2c:76:9c:f4"

namespace {
ControllerPtr controller = nullptr;
bool prevA = false;

// Accept only the configured MAC if provided, otherwise accept first controller.
bool macAllowed(const ControllerProperties& properties) {
  if (CONTROLLER_MAC[0] == '\0') return true;
  char addressStr[18];
  sprintf(addressStr, "%02x:%02x:%02x:%02x:%02x:%02x", properties.btaddr[0], properties.btaddr[1],
          properties.btaddr[2], properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
  return strcasecmp(addressStr, CONTROLLER_MAC) == 0;
}

void onConnectedController(ControllerPtr ctl) {
  const auto props = ctl->getProperties();
  char addressStr[18];
  sprintf(addressStr, "%02x:%02x:%02x:%02x:%02x:%02x", props.btaddr[0], props.btaddr[1], props.btaddr[2],
          props.btaddr[3], props.btaddr[4], props.btaddr[5]);
  Serial.printf("Controller tried to connect, MAC: %s\n", addressStr);

  if (!macAllowed(props)) {
    Serial.println("Rejected controller: MAC does not match CONTROLLER_MAC.");
    return;
  }
  controller = ctl;
  Serial.printf("Controller connected: %s (idx=%d)\n", ctl->getModelName().c_str(), ctl->index());
  ctl->setRumble(0x40, 250);
}

void onDisconnectedController(ControllerPtr ctl) {
  if (controller == ctl) {
    Serial.printf("Controller disconnected (idx=%d)\n", ctl->index());
    controller = nullptr;
  }
}
}  // namespace

void xboxInit() {
  // Bluepad32 v4.x API: provide connect/disconnect callbacks to setup().
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();  // ensure fresh pairing with the specified MAC
  Serial.println("Xbox controller support ready. Press A to flip clock direction.");
}

void xboxUpdate() {
  BP32.update();
  if (!controller || !controller->isConnected() || !controller->hasData()) {
    return;
  }

  bool a = controller->a();
  if (a && !prevA) {
    clockToggleDirection();
  }
  prevA = a;
}
