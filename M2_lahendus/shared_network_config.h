#ifndef SHARED_NETWORK_CONFIG_H
#define SHARED_NETWORK_CONFIG_H
 
// ---------------- WIFI / MQTT ----------------
//#define WIFI_SSID                           "Tanker"
//#define WIFI_PASS                           "tank5678"
#define WIFI_SSID                         "TalTech"
#define WIFI_PASS                         ""
 
#define MQTT_SERVER                         "193.40.245.72"
#define MQTT_PORT                           1883
#define MQTT_USER                           "test"
#define MQTT_PASS                           "test"
#define MQTT_PUBLISH_INTERVAL_MS            1000
 
// ---------------- CONVEYOR BELT — MAIN CONTROLLER ----------------
#define MAIN_CONTROLLER_CLIENT_NAME         "Main_controller"
#define MAIN_CONTROLLER_DEVICE_TOPIC        "Main_controller"
#define MAIN_CONTROLLER_STATUS_TOPIC        "Main_controller/status"
#define MAIN_CONTROLLER_COMMAND_TOPIC       "Main_controller/command"
 
// ---------------- CONVEYOR BELT — RESET BUTTON ----------------
#define RESET_BUTTON_CLIENT_NAME            "reset_button"
#define RESET_BUTTON_DEVICE_TOPIC           "reset_button"
#define RESET_BUTTON_STATUS_TOPIC           "reset_button/status"
 
// ---------------- CONVEYOR BELT — IMU CONTROLLER ----------------
#define IMU_CONTROLLER_CLIENT_NAME          "imu_controller"
#define IMU_CONTROLLER_DEVICE_TOPIC         "imu_controller"
#define IMU_CONTROLLER_STATUS_TOPIC         "imu_controller/status"
#define IMU_CONTROLLER_COMMAND_TOPIC        "imu_controller/command"
#define IMU_CONTROLLER_ROLL_TOPIC           "imu_controller/imu/roll"
 
// ---------------- BENDING MACHINE — RESET BUTTON 2 ----------------
#define RESET_BUTTON_2_CLIENT_NAME          "reset_button_2"
#define RESET_BUTTON_2_DEVICE_TOPIC         "reset_button_2"
#define RESET_BUTTON_2_STATUS_TOPIC         "reset_button_2/status"
 
// ---------------- BENDING MACHINE — SHARED TOPICS ----------------
// Used by reset_button_2 to listen for a future bending machine controller
// and by the dashboard to send commands to the bending machine.
#define BENDING_MACHINE_STATUS_TOPIC        "bending_machine/status"
#define BENDING_MACHINE_COMMAND_TOPIC       "bending_machine/command"
 
#endif