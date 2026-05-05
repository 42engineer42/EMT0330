#ifndef RESET_BUTTON_CONFIG_H
#define RESET_BUTTON_CONFIG_H

#include "../shared_network_config.h"
#include "../shared_maintenance_config.h"

// ---------------- BUTTON ----------------
#define BUTTON_PIN                0
#define BUTTON_ACTIVE_LOW         true
#define BUTTON_DEBOUNCE_MS        50

// ---------------- RGB LED ----------------
#define RGB_LED_PIN               38
#define RGB_LED_COUNT             1
#define RGB_LED_BRIGHTNESS        32

// ---------------- DEVICE TOPICS ----------------
#define MQTT_CLIENT_NAME          RESET_BUTTON_CLIENT_NAME
#define MQTT_DEVICE_TOPIC         RESET_BUTTON_DEVICE_TOPIC
#define MQTT_STATUS_TOPIC         RESET_BUTTON_STATUS_TOPIC
#define MQTT_SOURCE_TOPIC         MAIN_CONTROLLER_STATUS_TOPIC
#define MQTT_RESET_COMMAND_TOPIC  MAIN_CONTROLLER_COMMAND_TOPIC

#endif
