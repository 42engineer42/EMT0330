#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- LED STRIP ----------------
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ---------------- BUTTON ----------------
volatile bool buttonInterruptFired = false;
bool buttonPressed = false;
bool buttonWasDown = false;
bool maintenanceRequired = true;
bool maintenanceHoldHandled = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
const unsigned long maintenanceHoldTime = 1000;
unsigned long buttonPressStartTime = 0;

// ---------------- MOTOR STATE ----------------
int motorSpeed = 100;
String serialBuffer;
unsigned long lastLoopTime = 0;
unsigned long bootTime = 0;
unsigned long accumulatedWorkTimeMs = 0;
int adcSamples[ADC_FILTER_SAMPLES] = {0};
int adcSampleIndex = 0;
int adcSampleCount = 0;
long adcSampleSum = 0;

void IRAM_ATTR handleButtonInterrupt() {
  buttonInterruptFired = true;
}

float readCurrentMilliamps(int raw) {
  float voltageDelta = (raw - HALL_SENSOR_ZERO_ADC) *
                       (ADC_REFERENCE_VOLTAGE / ADC_MAX_READING);
  float currentAmps = voltageDelta / HALL_SENSOR_VOLTS_PER_AMP;

  if (currentAmps < 0.0f) {
    currentAmps = 0.0f;
  }

  return currentAmps * 1000.0f;
}

int filterAdcReading(int raw) {
  adcSampleSum -= adcSamples[adcSampleIndex];
  adcSamples[adcSampleIndex] = raw;
  adcSampleSum += raw;

  adcSampleIndex++;
  if (adcSampleIndex >= ADC_FILTER_SAMPLES) {
    adcSampleIndex = 0;
  }

  if (adcSampleCount < ADC_FILTER_SAMPLES) {
    adcSampleCount++;
  }

  return adcSampleSum / adcSampleCount;
}

void printDuration(unsigned long durationMs) {
  unsigned long totalSeconds = durationMs / 1000UL;
  unsigned long hours = totalSeconds / 3600UL;
  unsigned long minutes = (totalSeconds % 3600UL) / 60UL;
  unsigned long seconds = totalSeconds % 60UL;

  if (hours < 10) display.print('0');
  display.print(hours);
  display.print(':');
  if (minutes < 10) display.print('0');
  display.print(minutes);
  display.print(':');
  if (seconds < 10) display.print('0');
  display.print(seconds);
}

// ---------------- MOTOR CONTROL ----------------
void setMotor(int speed) {
  if (speed > 0) {
    digitalWrite(MOTOR_IN1_PIN, HIGH);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    ledcWrite(MOTOR_PWM_PIN, speed);
  } else if (speed < 0) {
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, HIGH);
    ledcWrite(MOTOR_PWM_PIN, -speed);
  } else {
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    ledcWrite(MOTOR_PWM_PIN, 0);
  }
}

bool isSignedInteger(const String &value) {
  if (value.length() == 0) {
    return false;
  }

  int start = 0;
  if (value[0] == '+' || value[0] == '-') {
    if (value.length() == 1) {
      return false;
    }
    start = 1;
  }

  for (int i = start; i < value.length(); i++) {
    if (!isDigit(value[i])) {
      return false;
    }
  }

  return true;
}

void applyMotorSpeed(int newSpeed, const char *source) {
  motorSpeed = constrain(newSpeed, -255, 255);
  Serial.print(source);
  Serial.print(" -> speed set to ");
  Serial.println(motorSpeed);
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    char incoming = static_cast<char>(Serial.read());

    if (incoming == '\r') {
      continue;
    }

    if (incoming != '\n') {
      serialBuffer += incoming;
      continue;
    }

    serialBuffer.trim();
    if (serialBuffer.length() == 0) {
      continue;
    }

    if (serialBuffer.equalsIgnoreCase("help")) {
      Serial.println("Enter speed from -255 to 255 and press enter.");
      Serial.println("Example: 120 or -200");
    } else if (isSignedInteger(serialBuffer)) {
      applyMotorSpeed(serialBuffer.toInt(), "Serial");
    } else {
      Serial.println("Invalid input. Type help or enter a number from -255 to 255.");
    }

    serialBuffer = "";
  }
}

// ---------------- BUTTON HANDLING ----------------
void updateButton() {
  unsigned long now = millis();
  bool pressedState = BUTTON_ACTIVE_LOW ? LOW : HIGH;
  bool releasedState = BUTTON_ACTIVE_LOW ? HIGH : LOW;
  bool isPressed = digitalRead(BUTTON_PIN) == pressedState;

  if (buttonInterruptFired) {
    buttonInterruptFired = false;

    if ((now - lastDebounceTime) < debounceDelay) {
      return;
    }

    if (isPressed) {
      buttonWasDown = true;
      maintenanceHoldHandled = false;
      buttonPressStartTime = now;
      lastDebounceTime = now;
    }
  }

  if (buttonWasDown && isPressed && !maintenanceHoldHandled &&
      (now - buttonPressStartTime) >= maintenanceHoldTime) {
    maintenanceRequired = false;
    maintenanceHoldHandled = true;
  }

  if (!buttonWasDown || isPressed || digitalRead(BUTTON_PIN) != releasedState) {
    return;
  }

  if ((now - lastDebounceTime) < debounceDelay) {
    return;
  }

  if (!maintenanceHoldHandled) {
    buttonPressed = true;
  }

  buttonWasDown = false;
  lastDebounceTime = now;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);

  // NEW LEDC API (ESP32 core v3.x)
  ledcAttach(MOTOR_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);

  // Current sense
  pinMode(CURRENT_SENSE_PIN, INPUT);

  // Button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                  handleButtonInterrupt,
                  BUTTON_ACTIVE_LOW ? FALLING : RISING);

  // I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // OLED init
  if (!display.begin(OLED_I2C_ADDRESS, true)) {
    Serial.println("OLED init failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  // LED strip
  strip.begin();
  strip.show();

  bootTime = millis();
  lastLoopTime = bootTime;

  Serial.println("Serial control ready. Enter speed from -255 to 255.");
}

// ---------------- LOOP ----------------
void loop() {
  handleSerialInput();
  updateButton();

  // Button cycles speed
  if (buttonPressed) {
    buttonPressed = false;

    int nextSpeed = motorSpeed + 50;
    if (nextSpeed > 255) nextSpeed = -255;
    applyMotorSpeed(nextSpeed, "Button");
  }

  setMotor(motorSpeed);

  int raw = analogRead(CURRENT_SENSE_PIN);
  int filteredRaw = filterAdcReading(raw);
  float currentMilliamps = readCurrentMilliamps(filteredRaw);
  unsigned long now = millis();
  unsigned long loopDelta = now - lastLoopTime;
  lastLoopTime = now;

  if (currentMilliamps > WORK_CURRENT_THRESHOLD_MA) {
    accumulatedWorkTimeMs += loopDelta;
  }

  unsigned long aliveTimeMs = now - bootTime;
  float workPercent = 0.0f;
  if (aliveTimeMs > 0) {
    workPercent = (100.0f * accumulatedWorkTimeMs) / aliveTimeMs;
  }

  // OLED display
  display.clearDisplay();
  display.setCursor(0, 0);

  display.print("Speed: ");
  display.println(motorSpeed);

  display.print("Curr: ");
  display.print(currentMilliamps, 0);
  display.println(" mA");

  display.print("ADC: ");
  display.println(raw);

  display.print("Flt: ");
  display.println(filteredRaw);

  display.print("Work: ");
  printDuration(accumulatedWorkTimeMs);
  display.println();

  display.print("Util: ");
  display.print(workPercent, 1);
  display.println("%");

  display.print("Maint: ");
  display.println(maintenanceRequired ? "YES" : "OK");

  display.display();

  // LED indication
  uint32_t color;
  if (motorSpeed > 0) {
    color = strip.Color(motorSpeed, 0, 0);   // red
  } else if (motorSpeed < 0) {
    color = strip.Color(0, 0, -motorSpeed);  // blue
  } else {
    color = strip.Color(0, 50, 0);           // green
  }

  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();

  delay(100);
}
