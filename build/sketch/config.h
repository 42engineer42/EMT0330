#line 1 "C:\\Users\\jaanr\\Documents\\GitHub\\EMT0330\\Binary_clock\\config.h"
#pragma once

#include <stdint.h>

// I2C pin configuration (change to match your board wiring).
constexpr int I2C_SDA_PIN = 34;
constexpr int I2C_SCL_PIN = 48;

// Default start time for the clock (HH, MM, SS).
struct ClockTime {
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

constexpr ClockTime kDefaultStart = {12, 0, 0};

// OLED configuration.
constexpr int kOledWidth = 128;
constexpr int kOledHeight = 64;
constexpr int kOledAddress = 0x3C;
constexpr int kOledResetPin = -1;
