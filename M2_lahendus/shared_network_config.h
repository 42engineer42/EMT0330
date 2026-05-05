#ifndef SHARED_NETWORK_CONFIG_H
#define SHARED_NETWORK_CONFIG_H

// ---------------- WIFI / MQTT ----------------
#define WIFI_SSID                        "Tanker"
#define WIFI_PASS                        "tank5678"
//#define WIFI_SSID                      "TalTech"
//#define WIFI_PASS                      ""

#define MQTT_SERVER                      "193.40.245.72"
#define MQTT_PORT                        1883
#define MQTT_USER                        "test"
#define MQTT_PASS                        "test"
#define MQTT_PUBLISH_INTERVAL_MS         1000

// ---------------- DEVICE / TOPIC NAMES ----------------
#define MAIN_CONTROLLER_CLIENT_NAME      "Main_controller"
#define MAIN_CONTROLLER_DEVICE_TOPIC     "Main_controller"
#define MAIN_CONTROLLER_STATUS_TOPIC     "Main_controller/status"
#define MAIN_CONTROLLER_COMMAND_TOPIC    "Main_controller/command"

#define RESET_BUTTON_CLIENT_NAME         "reset_button"
#define RESET_BUTTON_DEVICE_TOPIC        "reset_button"
#define RESET_BUTTON_STATUS_TOPIC        "reset_button/status"

#define IMU_CONTROLLER_CLIENT_NAME       "imu_controller"
#define IMU_CONTROLLER_DEVICE_TOPIC      "imu_controller"
#define IMU_CONTROLLER_STATUS_TOPIC      "imu_controller/status"
#define IMU_CONTROLLER_ROLL_TOPIC        "imu_controller/imu/roll"
#define MQTT_COMMAND_TOPIC               "imu_controller/command"
#define IMU_CONTROLLER_COMMAND_TOPIC     "imu_controller/command"

#define SECOND_CONTROLLER_CLIENT_NAME    "second_controller"
#define SECOND_CONTROLLER_DEVICE_TOPIC   "second_controller"
#define SECOND_CONTROLLER_STATUS_TOPIC   "second_controller/status"

#endif
