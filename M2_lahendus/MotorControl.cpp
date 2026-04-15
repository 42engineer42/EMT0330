#include "MotorControl.h"

static unsigned long lastChange = 0;
static uint8_t state = 0;

static const uint16_t TEST_DURATION = 1500;   // 1.5 seconds motor running
static const uint16_t STOP_DURATION = 600;    // 0.6 seconds pause between actions
static const uint16_t LOOP_DELAY   = 2000;    // 2 seconds after full test
static const uint8_t  SLOW         = 80;      // slow movement speed

void MotorControl::begin() {
  pinMode(LEFT_MOTOR_PWM, OUTPUT);
  pinMode(LEFT_MOTOR_DIR1, OUTPUT);
  pinMode(LEFT_MOTOR_DIR2, OUTPUT);

  pinMode(RIGHT_MOTOR_PWM, OUTPUT);
  pinMode(RIGHT_MOTOR_DIR1, OUTPUT);
  pinMode(RIGHT_MOTOR_DIR2, OUTPUT);
}

void MotorControl::stop() {
  analogWrite(LEFT_MOTOR_PWM, 0);
  analogWrite(RIGHT_MOTOR_PWM, 0);
}

void MotorControl::moveForward(uint8_t pwm) {
  leftForward(pwm);
  rightForward(pwm);
}

void MotorControl::moveBackward(uint8_t pwm) {
  leftBackward(pwm);
  rightBackward(pwm);
}

void MotorControl::turnRight(uint8_t leftSpeed, uint8_t rightSpeed) {
  leftForward(leftSpeed);
  rightForward(rightSpeed);
}

void MotorControl::turnLeft(uint8_t leftSpeed, uint8_t rightSpeed) {
  leftForward(leftSpeed);
  rightForward(rightSpeed);
}

void MotorControl::leftForward(uint8_t pwm) {
  analogWrite(LEFT_MOTOR_PWM, pwm);
  digitalWrite(LEFT_MOTOR_DIR1, LOW);
  digitalWrite(LEFT_MOTOR_DIR2, HIGH);
}

void MotorControl::leftBackward(uint8_t pwm) {
  analogWrite(LEFT_MOTOR_PWM, pwm);
  digitalWrite(LEFT_MOTOR_DIR1, HIGH);
  digitalWrite(LEFT_MOTOR_DIR2, LOW);
}

void MotorControl::rightForward(uint8_t pwm) {
  analogWrite(RIGHT_MOTOR_PWM, pwm);
  digitalWrite(RIGHT_MOTOR_DIR1, LOW);
  digitalWrite(RIGHT_MOTOR_DIR2, HIGH);
}

void MotorControl::rightBackward(uint8_t pwm) {
  analogWrite(RIGHT_MOTOR_PWM, pwm);
  digitalWrite(RIGHT_MOTOR_DIR1, HIGH);
  digitalWrite(RIGHT_MOTOR_DIR2, LOW);
}

void MotorControl::motorTestUpdate() {
    unsigned long now = millis();

    switch(state) {

        case 0:  // Start forward
            Serial.println(F("Test: Forward"));
            MotorControl::moveForward(SLOW);
            lastChange = now;
            state = 1;
            break;

        case 1:  // Hold forward
            if (now - lastChange >= TEST_DURATION) {
                MotorControl::stop();
                Serial.println(F("Stop"));
                lastChange = now;
                state = 2;
            }
            break;

        case 2:  // Pause
            if (now - lastChange >= STOP_DURATION) {
                Serial.println(F("Test: Backward"));
                MotorControl::moveBackward(SLOW);
                lastChange = now;
                state = 3;
            }
            break;

        case 3:  // Hold backward
            if (now - lastChange >= TEST_DURATION) {
                MotorControl::stop();
                Serial.println(F("Stop"));
                lastChange = now;
                state = 4;
            }
            break;

        case 4:  // Pause
            if (now - lastChange >= STOP_DURATION) {
                Serial.println(F("Test: Turn Left"));
                MotorControl::turnLeft(0, SLOW);
                lastChange = now;
                state = 5;
            }
            break;

        case 5:  // Hold turn left
            if (now - lastChange >= TEST_DURATION) {
                MotorControl::stop();
                Serial.println(F("Stop"));
                lastChange = now;
                state = 6;
            }
            break;

        case 6:  // Pause
            if (now - lastChange >= STOP_DURATION) {
                Serial.println(F("Test: Turn Right"));
                MotorControl::turnRight(SLOW, 0);
                lastChange = now;
                state = 7;
            }
            break;

        case 7:  // Hold turn right
            if (now - lastChange >= TEST_DURATION) {
                MotorControl::stop();
                Serial.println(F("Test Finished — waiting 2 seconds..."));
                lastChange = now;
                state = 8;
            }
            break;

        case 8:  // Wait 2 seconds, then restart test
            if (now - lastChange >= LOOP_DELAY) {
                Serial.println(F("Restarting motor test...\n"));
                state = 0;      // restart sequence
            }
            break;
    }
}