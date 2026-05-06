#ifndef CONFIG_H
#define CONFIG_H

#include "../shared_network_config.h"
#include "../shared_maintenance_config.h"

// ---------------- BUTTON ----------------
#define BUTTON_PIN               0
#define BUTTON_ACTIVE_LOW        true
#define BUTTON_DEBOUNCE_MS       50

// ---------------- RGB LED ----------------
#define RGB_LED_PIN              38
#define RGB_LED_COUNT            1
#define RGB_LED_BRIGHTNESS       80

// ---------------- DEVICE TOPICS ----------------
#define MQTT_CLIENT_NAME         RESET_BUTTON_2_CLIENT_NAME
#define MQTT_DEVICE_TOPIC        RESET_BUTTON_2_DEVICE_TOPIC
#define MQTT_STATUS_TOPIC        RESET_BUTTON_2_STATUS_TOPIC
#define MQTT_RESET_COMMAND_TOPIC BENDING_MACHINE_COMMAND_TOPIC

// This reset button monitors the bending machine controller.
// When a bending machine main controller is added, point MQTT_SOURCE_TOPIC
// to its status topic so tracked work time is inherited from that controller.
// For now it operates standalone (no source controller).
#define MQTT_SOURCE_TOPIC        BENDING_MACHINE_STATUS_TOPIC

#endif
