#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
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
 
// ---------------- NETWORK ----------------
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
unsigned long lastMqttPublishTime = 0;
unsigned long lastResetButtonStatusTime = 0;
unsigned long lastImuStatusTime = 0;
unsigned long lastSecondControllerStatusTime = 0;
 
// ---------------- BUTTON ----------------
volatile bool buttonInterruptFired = false;
bool buttonPressed = false;
bool buttonWasDown = false;
bool maintenanceRequired = false;
bool maintenanceHoldHandled = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
const unsigned long maintenanceHoldTime = 1000;
unsigned long buttonPressStartTime = 0;
 
// ---------------- IMU STATE ----------------
bool imuVibrationMaintenance = false;   // latched from IMU topic
 
// ---------------- MOTOR STATE ----------------
int requestedMotorSpeed = 200;
int appliedMotorSpeed = 0;
String serialBuffer;
unsigned long lastLoopTime = 0;
unsigned long bootTime = 0;
unsigned long accumulatedWorkTimeMs = 0;
unsigned long maintenanceResetOffsetMs = 0;
int adcSamples[ADC_FILTER_SAMPLES] = {0};
int adcSampleIndex = 0;
int adcSampleCount = 0;
long adcSampleSum = 0;
bool machineWorking = false;
bool resetButtonMaintenanceRequired = false;
bool resetButtonSourceMaintenance = false;
unsigned long resetButtonTrackedWorkMs = 0;
String resetButtonMachineState = "offline";
 
bool isControllerOnline(unsigned long now, unsigned long lastStatusTime) {
  return (now - lastStatusTime) <= (MQTT_PUBLISH_INTERVAL_MS * 3UL);
}
 
const unsigned long maintenanceIntervalMs =
    ((MAINTENANCE_INTERVAL_DAYS * 24UL + MAINTENANCE_INTERVAL_HOURS) * 60UL +
     MAINTENANCE_INTERVAL_MINUTES) * 60UL * 1000UL;
 
// Returns true if ANY maintenance condition is active
bool anyMaintenanceRequired() {
  return maintenanceRequired || imuVibrationMaintenance;
}
 
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
 
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println();
  Serial.print("WiFi connected, IP=");
  Serial.println(WiFi.localIP());
}
 
unsigned long parseUnsignedField(const String &payload,
                                 const char *fieldName,
                                 unsigned long fallbackValue) {
  String token = String("\"") + fieldName + "\":";
  int start = payload.indexOf(token);
  if (start < 0) return fallbackValue;
 
  start += token.length();
  while (start < payload.length() && payload[start] == ' ') start++;
 
  int end = start;
  while (end < payload.length() && isDigit(payload[end])) end++;
 
  if (end == start) return fallbackValue;
 
  return strtoul(payload.substring(start, end).c_str(), nullptr, 10);
}
 
bool parseBoolField(const String &payload, const char *fieldName, bool fallbackValue) {
  String token = String("\"") + fieldName + "\":";
  int start = payload.indexOf(token);
  if (start < 0) return fallbackValue;
 
  start += token.length();
  while (start < payload.length() && payload[start] == ' ') start++;
 
  if (payload.startsWith("true", start))  return true;
  if (payload.startsWith("false", start)) return false;
 
  return fallbackValue;
}
 
// Parse a nested bool: looks for "outer":{ ... "field": true/false ... }
bool parseNestedBoolField(const String &payload,
                          const char *outerKey,
                          const char *fieldName,
                          bool fallbackValue) {
  String outerToken = String("\"") + outerKey + "\":{";
  int outerStart = payload.indexOf(outerToken);
  if (outerStart < 0) return fallbackValue;
 
  int outerEnd = payload.indexOf('}', outerStart);
  if (outerEnd < 0) return fallbackValue;
 
  String sub = payload.substring(outerStart, outerEnd + 1);
  return parseBoolField(sub, fieldName, fallbackValue);
}
 
String parseStringField(const String &payload,
                        const char *fieldName,
                        const String &fallbackValue) {
  String token = String("\"") + fieldName + "\":\"";
  int start = payload.indexOf(token);
  if (start < 0) return fallbackValue;
 
  start += token.length();
  int end = payload.indexOf('"', start);
  if (end < 0) return fallbackValue;
 
  return payload.substring(start, end);
}
 
void updateMaintenanceState() {
  unsigned long trackedWorkMs = 0;
 
  if (accumulatedWorkTimeMs >= maintenanceResetOffsetMs) {
    trackedWorkMs = accumulatedWorkTimeMs - maintenanceResetOffsetMs;
  } else {
    maintenanceResetOffsetMs = accumulatedWorkTimeMs;
  }
 
  maintenanceRequired =
      (maintenanceIntervalMs > 0) && (trackedWorkMs >= maintenanceIntervalMs);
}
 
void publishImuReset();   // forward declaration
 
void resetAllMaintenance(const char *source) {
  maintenanceResetOffsetMs = accumulatedWorkTimeMs;
  maintenanceRequired = false;
  imuVibrationMaintenance = false;
  publishImuReset();
 
  Serial.print(source);
  Serial.println(" — all maintenance counters reset (motor + IMU vibration).");
}
 
// Keep old name as alias so nothing else breaks
void resetMaintenanceCounter(const char *source) {
  resetAllMaintenance(source);
}
 
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String topicName(topic);
  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }
 
  // ---- Command topic ----
  if (topicName == MQTT_COMMAND_TOPIC) {
    if (message.indexOf("reset_maintenance") >= 0) {
      resetMaintenanceCounter("MQTT");
    }
    return;
  }
 
  // ---- Reset button status ----
  if (topicName == MQTT_RESET_BUTTON_TOPIC) {
    resetButtonTrackedWorkMs =
        parseUnsignedField(message, "tracked_work_ms", resetButtonTrackedWorkMs);
    resetButtonMaintenanceRequired =
        parseBoolField(message, "maintenance_required", resetButtonMaintenanceRequired);
    resetButtonSourceMaintenance =
        parseBoolField(message, "source_controller_maintenance", resetButtonSourceMaintenance);
    resetButtonMachineState =
        parseStringField(message, "machine_state", resetButtonMachineState);
    lastResetButtonStatusTime = millis();
 
    Serial.print("MQTT ");
    Serial.print(MQTT_RESET_BUTTON_TOPIC);
    Serial.print(" -> ");
    Serial.println(message);
    return;
  }
 
  // ---- IMU status — parse vibration maintenance ----
  if (topicName == MQTT_IMU_STATUS_TOPIC) {
    lastImuStatusTime = millis();
 
    bool vibMaint = parseNestedBoolField(message, "vibration",
                                         "maintenance_required", false);
    if (vibMaint && !imuVibrationMaintenance) {
      imuVibrationMaintenance = true;
      Serial.println("IMU reported excessive vibration — maintenance required.");
    }
 
    Serial.print("MQTT ");
    Serial.print(MQTT_IMU_STATUS_TOPIC);
    Serial.print(" -> vib_maint=");
    Serial.println(vibMaint ? "true" : "false");
    return;
  }
 
  // ---- Second controller ----
  if (topicName == MQTT_SECOND_STATUS_TOPIC) {
    lastSecondControllerStatusTime = millis();
  }
}
 
void publishImuReset() {
  static const char resetPayload[] = "reset_maintenance";
  if (mqttClient.connected()) {
    if (mqttClient.publish(MQTT_IMU_COMMAND_TOPIC, resetPayload)) {
      Serial.print("MQTT ");
      Serial.print(MQTT_IMU_COMMAND_TOPIC);
      Serial.println(" -> reset_maintenance");
    } else {
      Serial.println("IMU reset publish failed");
    }
  }
}
 
void connectMqtt() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(512);
  mqttClient.setCallback(mqttCallback);
 
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT... ");
 
    String clientId = String(MQTT_CLIENT_NAME) + "-" +
                      String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
 
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      mqttClient.publish(MQTT_DEVICE_TOPIC, "online", true);
      mqttClient.subscribe(MQTT_COMMAND_TOPIC);
      mqttClient.subscribe(MQTT_RESET_BUTTON_TOPIC);
      mqttClient.subscribe(MQTT_IMU_STATUS_TOPIC);
      mqttClient.subscribe(MQTT_SECOND_STATUS_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 2s");
      delay(2000);
    }
  }
}
 
void ensureMqttConnection() {
  connectWiFi();
  if (!mqttClient.connected()) connectMqtt();
  mqttClient.loop();
}
 
void publishTelemetry(unsigned long now,
                      int raw,
                      int filteredRaw,
                      float currentMilliamps,
                      float workPercent) {
  char payload[512];
  unsigned long trackedWorkMs =
      (accumulatedWorkTimeMs >= maintenanceResetOffsetMs)
          ? (accumulatedWorkTimeMs - maintenanceResetOffsetMs)
          : 0;
 
  const bool maint = anyMaintenanceRequired();
 
  const char *machineState =
      maint          ? "maintenance" :
      machineWorking ? "working"     : "idle";
 
  int len = snprintf(payload, sizeof(payload),
                     "{\"ms\":%lu,\"speed\":%d,\"current_ma\":%.1f,"
                     "\"adc_raw\":%d,\"adc_filtered\":%d,"
                     "\"work_ms\":%lu,\"tracked_work_ms\":%lu,"
                     "\"requested_speed\":%d,"
                     "\"maintenance_threshold_ms\":%lu,\"util_pct\":%.1f,"
                     "\"maintenance_required\":%s,"
                     "\"imu_vibration_maintenance\":%s,"
                     "\"machine_state\":\"%s\"}",
                     now,
                     appliedMotorSpeed,
                     currentMilliamps,
                     raw,
                     filteredRaw,
                     accumulatedWorkTimeMs,
                     trackedWorkMs,
                     requestedMotorSpeed,
                     maintenanceIntervalMs,
                     workPercent,
                     maintenanceRequired        ? "true" : "false",
                     imuVibrationMaintenance    ? "true" : "false",
                     machineState);
 
  if (len < 0 || len >= static_cast<int>(sizeof(payload))) {
    Serial.println("MQTT telemetry payload truncated");
    return;
  }
 
  if (mqttClient.publish(MQTT_STATE_TOPIC, payload)) {
    Serial.print("MQTT ");
    Serial.print(MQTT_STATE_TOPIC);
    Serial.print(" -> ");
    Serial.println(payload);
  } else {
    Serial.println("MQTT publish failed");
  }
}
 
void IRAM_ATTR handleButtonInterrupt() {
  buttonInterruptFired = true;
}
 
float readCurrentMilliamps(int raw) {
  float voltageDelta = (raw - HALL_SENSOR_ZERO_ADC) *
                       (ADC_REFERENCE_VOLTAGE / ADC_MAX_READING);
  float currentAmps = voltageDelta / HALL_SENSOR_VOLTS_PER_AMP;
  if (currentAmps < 0.0f) currentAmps = 0.0f;
  return currentAmps * 1000.0f;
}
 
int filterAdcReading(int raw) {
  adcSampleSum -= adcSamples[adcSampleIndex];
  adcSamples[adcSampleIndex] = raw;
  adcSampleSum += raw;
 
  adcSampleIndex++;
  if (adcSampleIndex >= ADC_FILTER_SAMPLES) adcSampleIndex = 0;
  if (adcSampleCount < ADC_FILTER_SAMPLES) adcSampleCount++;
 
  return adcSampleSum / adcSampleCount;
}
 
void printDuration(unsigned long durationMs) {
  unsigned long totalSeconds = durationMs / 1000UL;
  unsigned long hours   = totalSeconds / 3600UL;
  unsigned long minutes = (totalSeconds % 3600UL) / 60UL;
  unsigned long seconds = totalSeconds % 60UL;
 
  if (hours < 10)   display.print('0');
  display.print(hours);   display.print(':');
  if (minutes < 10) display.print('0');
  display.print(minutes); display.print(':');
  if (seconds < 10) display.print('0');
  display.print(seconds);
}
 
void renderDebugDisplay(int motorSpeedValue, int rawAdcValue, int filteredAdcValue) {
  display.print("Speed: "); display.println(motorSpeedValue);
  display.print("ADC: ");   display.println(rawAdcValue);
  display.print("Flt: ");   display.println(filteredAdcValue);
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
  if (value.length() == 0) return false;
  int start = 0;
  if (value[0] == '+' || value[0] == '-') {
    if (value.length() == 1) return false;
    start = 1;
  }
  for (int i = start; i < value.length(); i++) {
    if (!isDigit(value[i])) return false;
  }
  return true;
}
 
void applyMotorSpeed(int newSpeed, const char *source) {
  requestedMotorSpeed = constrain(newSpeed, -255, 255);
  Serial.print(source);
  Serial.print(" -> speed set to ");
  Serial.println(requestedMotorSpeed);
}
 
void handleSerialInput() {
  while (Serial.available() > 0) {
    char incoming = static_cast<char>(Serial.read());
    if (incoming == '\r') continue;
    if (incoming != '\n') { serialBuffer += incoming; continue; }
 
    serialBuffer.trim();
    if (serialBuffer.length() == 0) { serialBuffer = ""; continue; }
 
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
  bool pressedState  = BUTTON_ACTIVE_LOW ? LOW  : HIGH;
  bool releasedState = BUTTON_ACTIVE_LOW ? HIGH : LOW;
  bool isPressed = digitalRead(BUTTON_PIN) == pressedState;
 
  if (buttonInterruptFired) {
    buttonInterruptFired = false;
 
    if ((now - lastDebounceTime) < debounceDelay) return;
 
    if (isPressed) {
      buttonWasDown = true;
      maintenanceHoldHandled = false;
      buttonPressStartTime = now;
      lastDebounceTime = now;
    }
  }
 
  if (buttonWasDown && isPressed && !maintenanceHoldHandled &&
      (now - buttonPressStartTime) >= maintenanceHoldTime) {
    resetMaintenanceCounter("Main controller button");
    maintenanceHoldHandled = true;
  }
 
  if (!buttonWasDown || isPressed || digitalRead(BUTTON_PIN) != releasedState) return;
  if ((now - lastDebounceTime) < debounceDelay) return;
 
  if (!maintenanceHoldHandled) buttonPressed = true;
 
  buttonWasDown = false;
  lastDebounceTime = now;
}
 
// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
 
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  ledcAttach(MOTOR_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
 
  pinMode(CURRENT_SENSE_PIN, INPUT);
 
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                  handleButtonInterrupt,
                  BUTTON_ACTIVE_LOW ? FALLING : RISING);
 
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
 
  if (!display.begin(OLED_I2C_ADDRESS, true)) {
    Serial.println("OLED init failed");
    while (true);
  }
 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
 
  strip.begin();
  strip.show();
 
  bootTime = millis();
  lastLoopTime = bootTime;
 
  connectWiFi();
  connectMqtt();
 
  Serial.println("Serial control ready. Enter speed from -255 to 255.");
  Serial.print("Maintenance interval (ms): ");
  Serial.println(maintenanceIntervalMs);
}
 
// ---------------- LOOP ----------------
void loop() {
  ensureMqttConnection();
  handleSerialInput();
  updateButton();
 
  // Button short press cycles speed
  if (buttonPressed) {
    buttonPressed = false;
    int nextSpeed = requestedMotorSpeed + 50;
    if (nextSpeed > 255) nextSpeed = -255;
    applyMotorSpeed(nextSpeed, "Button");
  }
 
  // Stop motor if ANY maintenance condition is active
  appliedMotorSpeed = anyMaintenanceRequired() ? 0 : requestedMotorSpeed;
  setMotor(appliedMotorSpeed);
 
  int raw = analogRead(CURRENT_SENSE_PIN);
  int filteredRaw = filterAdcReading(raw);
  float currentMilliamps = readCurrentMilliamps(filteredRaw);
  unsigned long now = millis();
  unsigned long loopDelta = now - lastLoopTime;
  lastLoopTime = now;
 
  const bool motorCommandedToRun = appliedMotorSpeed != 0;
  machineWorking = motorCommandedToRun &&
                   (currentMilliamps > WORK_CURRENT_THRESHOLD_MA);
 
  if (machineWorking) accumulatedWorkTimeMs += loopDelta;
 
  updateMaintenanceState();
 
  unsigned long aliveTimeMs = now - bootTime;
  float workPercent = 0.0f;
  if (aliveTimeMs > 0) {
    workPercent = (100.0f * accumulatedWorkTimeMs) / aliveTimeMs;
  }
 
  // ---- OLED ----
  display.clearDisplay();
  display.setCursor(0, 0);
 
  const bool maint = anyMaintenanceRequired();
 
  display.print("Curr: ");
  display.print(currentMilliamps, 0);
  display.print(" M:");
  display.println(maint ? "YES" : "OK");
 
  display.print("Util: ");
  display.print(workPercent, 1);
  display.println("%");
 
  display.print("Work: ");
  printDuration(accumulatedWorkTimeMs);
  display.println();
 
  display.print("Trk: ");
  printDuration(accumulatedWorkTimeMs - maintenanceResetOffsetMs);
  display.println();
 
  display.print("Vib: ");
  display.println(imuVibrationMaintenance ? "MAINT" : "OK");
 
  const bool mainOnline   = (WiFi.status() == WL_CONNECTED) && mqttClient.connected();
  const bool resetOnline  = isControllerOnline(now, lastResetButtonStatusTime);
  const bool imuOnline    = isControllerOnline(now, lastImuStatusTime);
  const bool secondOnline = isControllerOnline(now, lastSecondControllerStatusTime);
 
  display.print("M:");  display.print(mainOnline   ? "Y" : "N");
  display.print(" R:"); display.print(resetOnline  ? "Y" : "N");
  display.print(" I:"); display.print(imuOnline    ? "Y" : "N");
  display.print(" 2:"); display.println(secondOnline ? "Y" : "N");
 
  display.display();
 
  // ---- LED strip ----
  uint32_t color;
  if (anyMaintenanceRequired()) {
    color = strip.Color(50, 0, 0);     // red — any maintenance (motor or vibration)
  } else if (machineWorking) {
    color = strip.Color(0, 50, 0);     // green — working
  } else {
    color = strip.Color(50, 30, 0);    // yellow — idle
  }
 
  for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
  strip.show();
 
  if ((now - lastMqttPublishTime) >= MQTT_PUBLISH_INTERVAL_MS) {
    lastMqttPublishTime = now;
    publishTelemetry(now, raw, filteredRaw, currentMilliamps, workPercent);
  }
 
  delay(100);
}