#ifndef CONFIG_H
#define CONFIG_H

#include "../shared_network_config.h"
#include "../shared_maintenance_config.h"

// ---------------- MOTOR DRIVER (TB6612FNG) ----------------
#define MOTOR_PWM_PIN      39   // Speed control (PWM)
#define MOTOR_IN1_PIN      36   // Direction pin 1
#define MOTOR_IN2_PIN      37   // Direction pin 2
// NOTE: STBY must be HIGH (tie to 3.3V or control via GPIO)

// ---------------- CURRENT SENSING ----------------
#define CURRENT_SENSE_PIN  5    // Analog input
#define ADC_REFERENCE_VOLTAGE     5.0f
#define ADC_MAX_READING           4095.0f
#define ADC_FILTER_SAMPLES        30
#define HALL_SENSOR_ZERO_ADC      2025.0f // 0 A ADC reading
#define HALL_SENSOR_VOLTS_PER_AMP 0.2f  // Adjust to your sensor
#define WORK_CURRENT_THRESHOLD_MA 2

// ---------------- OLED DISPLAY (I2C) ----------------
#define I2C_SDA_PIN        8
#define I2C_SCL_PIN        9

// Placeholder for display I2C address
#define OLED_I2C_ADDRESS   0x3C   // <-- CHANGE IF NEEDED

// ---------------- WS2812B LED STRIP ----------------
#define LED_PIN            38
#define LED_COUNT          7

// ---------------- BUTTON ----------------
#define BUTTON_PIN         0
#define BUTTON_ACTIVE_LOW  true

// ---------------- PWM SETTINGS ----------------
#define PWM_CHANNEL        0
#define PWM_FREQ           20000   // 20 kHz
#define PWM_RESOLUTION     8       // 8-bit (0–255)

// ---------------- DEVICE TOPICS ----------------
#define MQTT_CLIENT_NAME           MAIN_CONTROLLER_CLIENT_NAME
#define MQTT_DEVICE_TOPIC          MAIN_CONTROLLER_DEVICE_TOPIC
#define MQTT_STATE_TOPIC           MAIN_CONTROLLER_STATUS_TOPIC
#define MQTT_COMMAND_TOPIC         MAIN_CONTROLLER_COMMAND_TOPIC
#define MQTT_RESET_BUTTON_TOPIC    RESET_BUTTON_STATUS_TOPIC
#define MQTT_IMU_STATUS_TOPIC      IMU_CONTROLLER_STATUS_TOPIC
#define MQTT_SECOND_STATUS_TOPIC   SECOND_CONTROLLER_STATUS_TOPIC
#define MQTT_IMU_COMMAND_TOPIC     IMU_CONTROLLER_COMMAND_TOPIC

#endif
