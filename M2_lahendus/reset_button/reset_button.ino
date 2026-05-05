#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Adafruit_NeoPixel rgbLed(RGB_LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

volatile bool buttonInterruptFired = false;
bool buttonWasDown = false;
bool buttonPressed = false;
unsigned long lastDebounceTime = 0;

unsigned long lastMqttPublishTime = 0;
unsigned long lastStatusReceiveTime = 0;
unsigned long latestWorkTimeMs = 0;
unsigned long lastWorkTimeMs = 0;
unsigned long maintenanceResetOffsetMs = 0;

bool mainControllerMaintenanceFlag = false;
bool localMaintenanceRequired = false;
bool machineWorking = false;
bool hasTelemetry = false;

const unsigned long kMaintenanceIntervalMs =
    ((MAINTENANCE_INTERVAL_DAYS * 24UL + MAINTENANCE_INTERVAL_HOURS) * 60UL +
     MAINTENANCE_INTERVAL_MINUTES) * 60UL * 1000UL;

void publishMainControllerReset();

void IRAM_ATTR handleButtonInterrupt() {
  buttonInterruptFired = true;
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

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

void setStatusLed(uint8_t red, uint8_t green, uint8_t blue) {
  for (uint16_t i = 0; i < RGB_LED_COUNT; i++) {
    rgbLed.setPixelColor(i, rgbLed.Color(red, green, blue));
  }
  rgbLed.show();
}

void updateMaintenanceState() {
  unsigned long trackedWorkMs = 0;

  if (latestWorkTimeMs >= maintenanceResetOffsetMs) {
    trackedWorkMs = latestWorkTimeMs - maintenanceResetOffsetMs;
  } else {
    maintenanceResetOffsetMs = latestWorkTimeMs;
  }

  localMaintenanceRequired =
      (kMaintenanceIntervalMs > 0) && (trackedWorkMs >= kMaintenanceIntervalMs);
}

unsigned long parseUnsignedField(const String &payload,
                                 const char *fieldName,
                                 unsigned long fallbackValue) {
  String token = String("\"") + fieldName + "\":";
  int start = payload.indexOf(token);
  if (start < 0) {
    return fallbackValue;
  }

  start += token.length();
  while (start < payload.length() && payload[start] == ' ') {
    start++;
  }

  int end = start;
  while (end < payload.length() && isDigit(payload[end])) {
    end++;
  }

  if (end == start) {
    return fallbackValue;
  }

  return strtoul(payload.substring(start, end).c_str(), nullptr, 10);
}

bool parseBoolField(const String &payload, const char *fieldName, bool fallbackValue) {
  String token = String("\"") + fieldName + "\":";
  int start = payload.indexOf(token);
  if (start < 0) {
    return fallbackValue;
  }

  start += token.length();
  while (start < payload.length() && payload[start] == ' ') {
    start++;
  }

  if (payload.startsWith("true", start)) {
    return true;
  }

  if (payload.startsWith("false", start)) {
    return false;
  }

  return fallbackValue;
}

void publishStatus() {
  char payload[256];
  unsigned long trackedWorkMs =
      (latestWorkTimeMs >= maintenanceResetOffsetMs)
          ? (latestWorkTimeMs - maintenanceResetOffsetMs)
          : 0;

  const char *machineState = "idle";
  if (localMaintenanceRequired) {
    machineState = "maintenance";
  } else if (machineWorking) {
    machineState = "working";
  }

  snprintf(payload,
           sizeof(payload),
           "{\"tracked_work_ms\":%lu,\"maintenance_threshold_ms\":%lu,"
           "\"maintenance_required\":%s,\"source_controller_maintenance\":%s,"
           "\"machine_state\":\"%s\"}",
           trackedWorkMs,
           kMaintenanceIntervalMs,
           localMaintenanceRequired ? "true" : "false",
           mainControllerMaintenanceFlag ? "true" : "false",
           machineState);

  mqttClient.publish(MQTT_STATUS_TOPIC, payload, true);
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String topicName(topic);
  if (topicName != MQTT_SOURCE_TOPIC) {
    return;
  }

  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }

  unsigned long newWorkTimeMs =
      parseUnsignedField(message, "work_ms", latestWorkTimeMs);
  bool reportedMaintenance =
      parseBoolField(message, "maintenance_required", false);

  if (!hasTelemetry) {
    maintenanceResetOffsetMs = newWorkTimeMs;
  } else if (newWorkTimeMs < latestWorkTimeMs) {
    maintenanceResetOffsetMs = newWorkTimeMs;
  }

  lastWorkTimeMs = latestWorkTimeMs;
  latestWorkTimeMs = newWorkTimeMs;
  machineWorking = hasTelemetry && (latestWorkTimeMs > lastWorkTimeMs);
  mainControllerMaintenanceFlag = reportedMaintenance;
  hasTelemetry = true;
  lastStatusReceiveTime = millis();

  updateMaintenanceState();

  Serial.print("MQTT ");
  Serial.print(MQTT_SOURCE_TOPIC);
  Serial.print(" -> ");
  Serial.println(message);
}

void connectMqtt() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(256);
  mqttClient.setCallback(mqttCallback);

  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT... ");

    String clientId = String(MQTT_CLIENT_NAME) + "-" +
                      String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      mqttClient.subscribe(MQTT_SOURCE_TOPIC);
      mqttClient.publish(MQTT_DEVICE_TOPIC, "online", true);
      publishStatus();
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

  if (!mqttClient.connected()) {
    connectMqtt();
  }

  mqttClient.loop();
}

void updateButton() {
  unsigned long now = millis();
  bool pressedState = BUTTON_ACTIVE_LOW ? LOW : HIGH;
  bool releasedState = BUTTON_ACTIVE_LOW ? HIGH : LOW;
  bool isPressed = digitalRead(BUTTON_PIN) == pressedState;

  if (buttonInterruptFired) {
    buttonInterruptFired = false;

    if ((now - lastDebounceTime) >= BUTTON_DEBOUNCE_MS && isPressed) {
      buttonWasDown = true;
      lastDebounceTime = now;
    }
  }

  if (!buttonWasDown || isPressed || digitalRead(BUTTON_PIN) != releasedState) {
    return;
  }

  if ((now - lastDebounceTime) < BUTTON_DEBOUNCE_MS) {
    return;
  }

  buttonWasDown = false;
  buttonPressed = true;
  lastDebounceTime = now;
}

void handleResetRequest() {
  if (!buttonPressed) {
    return;
  }

  buttonPressed = false;
  maintenanceResetOffsetMs = latestWorkTimeMs;
  localMaintenanceRequired = false;
  publishMainControllerReset();

  Serial.println("Maintenance counter reset from boot button.");
  publishStatus();
}

void publishMainControllerReset() {
  static const char payload[] = "reset_maintenance";

  if (mqttClient.publish(MQTT_RESET_COMMAND_TOPIC, payload, true)) {
    Serial.print("MQTT ");
    Serial.print(MQTT_RESET_COMMAND_TOPIC);
    Serial.print(" -> ");
    Serial.println(payload);
  } else {
    Serial.println("Main controller reset publish failed");
  }
}

void updateLedState() {
  if (localMaintenanceRequired) {
    setStatusLed(255, 0, 0);
    return;
  }

  if (machineWorking) {
    setStatusLed(0, 255, 0);
    return;
  }

  setStatusLed(255, 160, 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                  handleButtonInterrupt,
                  BUTTON_ACTIVE_LOW ? FALLING : RISING);

  rgbLed.begin();
  rgbLed.setBrightness(RGB_LED_BRIGHTNESS);
  rgbLed.clear();
  setStatusLed(255, 160, 0);

  connectWiFi();
  connectMqtt();

  Serial.println("reset_button ready.");
  Serial.print("Maintenance interval (ms): ");
  Serial.println(kMaintenanceIntervalMs);
}

void loop() {
  ensureMqttConnection();
  updateButton();
  handleResetRequest();
  updateMaintenanceState();
  updateLedState();

  unsigned long now = millis();
  if ((now - lastMqttPublishTime) >= MQTT_PUBLISH_INTERVAL_MS) {
    lastMqttPublishTime = now;
    publishStatus();
  }

  delay(20);
}
