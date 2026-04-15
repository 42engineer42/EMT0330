#pragma once
#include <Arduino.h>
#include "Config.h"

class MotorControl {
public:
  static void begin();
  static void moveForward(uint8_t pwm);
  static void moveBackward(uint8_t pwm);
  static void turnRight(uint8_t leftSpeed, uint8_t rightSpeed);
  static void turnLeft(uint8_t leftSpeed, uint8_t rightSpeed);
  static void motorTestUpdate();
  static void stop();

private:
  // Vasak mootor
  static void leftForward(uint8_t pwm);
  static void leftBackward(uint8_t pwm);

  // Parem mootor
  static void rightForward(uint8_t pwm);
  static void rightBackward(uint8_t pwm);
};