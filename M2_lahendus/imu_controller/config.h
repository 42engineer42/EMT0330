#ifndef IMU_CONTROLLER_CONFIG_H
#define IMU_CONTROLLER_CONFIG_H

#include "../shared_network_config.h"
#include "../shared_maintenance_config.h"

// ---------------- I2C ----------------
#define I2C_SDA_PIN              42
#define I2C_SCL_PIN              41
#define I2C_CLOCK_HZ             100000

// ---------------- SENSOR ADDRESSES ----------------
#define BME280_I2C_ADDRESS       0x76
#define BNO08X_I2C_ADDRESS       0x4A
#define SEA_LEVEL_PRESSURE_HPA   1013.25f

// ---------------- TIMING ----------------
#define SERIAL_PRINT_INTERVAL_MS 1000UL

// ---------------- DEVICE TOPICS ----------------
#define MQTT_CLIENT_NAME         IMU_CONTROLLER_CLIENT_NAME
#define MQTT_DEVICE_TOPIC        IMU_CONTROLLER_DEVICE_TOPIC
#define MQTT_STATUS_TOPIC        IMU_CONTROLLER_STATUS_TOPIC
#define MQTT_ROLL_TOPIC          IMU_CONTROLLER_ROLL_TOPIC

#endif
