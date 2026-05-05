#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BNO08x.h>
#include "config.h"
 
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
 
Adafruit_BME280 bme;
Adafruit_BNO08x bno08x(-1);
 
// ---------------- IMU DATA ----------------
struct ImuData {
  float ax = NAN, ay = NAN, az = NAN;
  float gx = NAN, gy = NAN, gz = NAN;
  float lx = NAN, ly = NAN, lz = NAN;
  float gravx = NAN, gravy = NAN, gravz = NAN;
  float qw = NAN, qx = NAN, qy = NAN, qz = NAN;
  float yaw = NAN, pitch = NAN, roll = NAN;
} imu;
 
// ---------------- VIBRATION DETECTION ----------------
// RMS of linear acceleration magnitude over a sliding window
// If RMS > threshold for >= VIBRATION_DURATION_MS → require maintenance
 
static const float  VIBRATION_THRESHOLD_M_S2 = 2.0f;  // m/s² RMS — tune to your machine
static const unsigned long VIBRATION_DURATION_MS = 2000UL;
 
// Sliding window for RMS (sampled each loop tick when IMU data is fresh)
static const int VIB_WINDOW = 20;
float vibWindow[VIB_WINDOW] = {0};
int   vibIdx = 0;
bool  vibWindowFull = false;
 
unsigned long vibExceedStart = 0;  // millis() when RMS first exceeded threshold
bool vibExceeding  = false;        // RMS currently above threshold
bool vibMaintenance = false;       // latched — requires explicit reset
 
float vibrationRms() {
  int count = vibWindowFull ? VIB_WINDOW : vibIdx;
  if (count == 0) return 0.0f;
  float sum = 0.0f;
  for (int i = 0; i < count; i++) sum += vibWindow[i] * vibWindow[i];
  return sqrtf(sum / count);
}
 
void updateVibration(unsigned long now) {
  if (isnan(imu.lx) || isnan(imu.ly) || isnan(imu.lz)) return;
 
  float mag = sqrtf(imu.lx * imu.lx + imu.ly * imu.ly + imu.lz * imu.lz);
 
  vibWindow[vibIdx] = mag;
  vibIdx = (vibIdx + 1) % VIB_WINDOW;
  if (vibIdx == 0) vibWindowFull = true;
 
  float rms = vibrationRms();
 
  if (rms > VIBRATION_THRESHOLD_M_S2) {
    if (!vibExceeding) {
      vibExceeding = true;
      vibExceedStart = now;
    } else if (!vibMaintenance &&
               (now - vibExceedStart) >= VIBRATION_DURATION_MS) {
      vibMaintenance = true;
      Serial.printf("VIBRATION: RMS=%.2f m/s² exceeded threshold for %lu ms — maintenance required\n",
                    rms, now - vibExceedStart);
    }
  } else {
    vibExceeding = false;
    // Note: vibMaintenance stays latched until explicit reset via MQTT command
  }
}
 
// ---------------- TIMING ----------------
unsigned long lastSerialPrintTime  = 0;
unsigned long lastMqttPublishTime  = 0;
unsigned long lastVibPublishTime   = 0;
 
// ---------------- HELPERS ----------------
void quaternionToEuler(float qr, float qi, float qj, float qk,
                       float &yaw, float &pitch, float &roll) {
  const float sqr = sq(qr), sqi = sq(qi), sqj = sq(qj), sqk = sq(qk);
  const float norm = sqi + sqj + sqk + sqr;
 
  yaw   = atan2f(2.0f * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr));
  pitch = asinf(-2.0f * (qi * qk - qj * qr) / norm);
  roll  = atan2f(2.0f * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr));
 
  yaw   *= RAD_TO_DEG;
  pitch *= RAD_TO_DEG;
  roll  *= RAD_TO_DEG;
}
 
void setBnoReports() {
  if (!bno08x.enableReport(SH2_ACCELEROMETER))
    Serial.println("Failed to enable accelerometer report");
  if (!bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED))
    Serial.println("Failed to enable gyroscope report");
  if (!bno08x.enableReport(SH2_LINEAR_ACCELERATION))
    Serial.println("Failed to enable linear acceleration report");
  if (!bno08x.enableReport(SH2_GRAVITY))
    Serial.println("Failed to enable gravity report");
  if (!bno08x.enableReport(SH2_ROTATION_VECTOR))
    Serial.println("Failed to enable rotation vector report");
}
 
void updateImuFromEvent(const sh2_SensorValue_t &sensorValue) {
  switch (sensorValue.sensorId) {
    case SH2_ACCELEROMETER:
      imu.ax = sensorValue.un.accelerometer.x;
      imu.ay = sensorValue.un.accelerometer.y;
      imu.az = sensorValue.un.accelerometer.z;
      break;
    case SH2_GYROSCOPE_CALIBRATED:
      imu.gx = sensorValue.un.gyroscope.x;
      imu.gy = sensorValue.un.gyroscope.y;
      imu.gz = sensorValue.un.gyroscope.z;
      break;
    case SH2_LINEAR_ACCELERATION:
      imu.lx = sensorValue.un.linearAcceleration.x;
      imu.ly = sensorValue.un.linearAcceleration.y;
      imu.lz = sensorValue.un.linearAcceleration.z;
      break;
    case SH2_GRAVITY:
      imu.gravx = sensorValue.un.gravity.x;
      imu.gravy = sensorValue.un.gravity.y;
      imu.gravz = sensorValue.un.gravity.z;
      break;
    case SH2_ROTATION_VECTOR:
      imu.qw = sensorValue.un.rotationVector.real;
      imu.qx = sensorValue.un.rotationVector.i;
      imu.qy = sensorValue.un.rotationVector.j;
      imu.qz = sensorValue.un.rotationVector.k;
      quaternionToEuler(imu.qw, imu.qx, imu.qy, imu.qz,
                        imu.yaw, imu.pitch, imu.roll);
      break;
    default:
      break;
  }
}
 
// ---------------- WIFI / MQTT ----------------
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
 
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String topicStr(topic);
  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }
 
  if (topicStr == MQTT_COMMAND_TOPIC) {
    if (message.indexOf("reset_maintenance") >= 0) {
      vibMaintenance = false;
      vibExceeding   = false;
      vibExceedStart = 0;
      Serial.println("Vibration maintenance flag reset via MQTT.");
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
 
// ---------------- SERIAL ----------------
void printHeader() {
  Serial.println(
      "ms,temp_c,pressure_hpa,humidity_pct,alt_m,"
      "accel_x,accel_y,accel_z,"
      "gyro_x,gyro_y,gyro_z,"
      "lin_x,lin_y,lin_z,"
      "grav_x,grav_y,grav_z,"
      "yaw_deg,pitch_deg,roll_deg,"
      "vib_rms,vib_maintenance");
}
 
// ---------------- PUBLISH ----------------
void publishRoll(float rollDeg) {
  char payload[24];
  snprintf(payload, sizeof(payload), "%.2f", rollDeg);
 
  if (mqttClient.publish(MQTT_ROLL_TOPIC, payload)) {
    Serial.print("MQTT ");
    Serial.print(MQTT_ROLL_TOPIC);
    Serial.print(" -> ");
    Serial.println(payload);
  } else {
    Serial.println("MQTT roll publish failed");
  }
}
 
void publishTelemetry(unsigned long now,
                      float temperatureC,
                      float pressureHpa,
                      float humidityPct,
                      float altitudeM) {
  float rms = vibrationRms();
 
  char payload[640];
  snprintf(payload, sizeof(payload),
           "{\"ms\":%lu,\"temp_c\":%.2f,\"pressure_hpa\":%.2f,"
           "\"humidity_pct\":%.2f,\"alt_m\":%.2f,"
           "\"accel\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
           "\"gyro\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
           "\"linear\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
           "\"gravity\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
           "\"orientation\":{\"yaw\":%.2f,\"pitch\":%.2f,\"roll\":%.2f},"
           "\"vibration\":{\"rms\":%.3f,\"exceeding\":%s,\"maintenance_required\":%s}}",
           now,
           temperatureC, pressureHpa, humidityPct, altitudeM,
           imu.ax, imu.ay, imu.az,
           imu.gx, imu.gy, imu.gz,
           imu.lx, imu.ly, imu.lz,
           imu.gravx, imu.gravy, imu.gravz,
           imu.yaw, imu.pitch, imu.roll,
           rms,
           vibExceeding   ? "true" : "false",
           vibMaintenance ? "true" : "false");
 
  if (mqttClient.publish(MQTT_STATUS_TOPIC, payload)) {
    Serial.print("MQTT ");
    Serial.print(MQTT_STATUS_TOPIC);
    Serial.print(" -> ");
    Serial.println(payload);
  } else {
    Serial.println("MQTT telemetry publish failed");
  }
}
 
// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(1000);
 
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(I2C_CLOCK_HZ);
  delay(100);
 
  Serial.println();
  Serial.println("imu_controller starting");
 
  if (!bme.begin(BME280_I2C_ADDRESS, &Wire)) {
    Serial.print("BME280 begin() failed, sensorID=0x");
    Serial.println(bme.sensorID(), HEX);
    while (true) delay(100);
  }
 
  Serial.print("BME280 OK, sensorID=0x");
  Serial.println(bme.sensorID(), HEX);
 
  bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                  Adafruit_BME280::SAMPLING_X16,
                  Adafruit_BME280::SAMPLING_X16,
                  Adafruit_BME280::SAMPLING_X16,
                  Adafruit_BME280::FILTER_X16,
                  Adafruit_BME280::STANDBY_MS_500);
  delay(100);
 
  if (!bno08x.begin_I2C(BNO08X_I2C_ADDRESS, &Wire)) {
    Serial.println("BNO08x begin_I2C() failed");
    while (true) delay(100);
  }
 
  Serial.println("BNO08x OK");
  setBnoReports();
 
  connectWiFi();
  connectMqtt();
  printHeader();
}
 
// ---------------- LOOP ----------------
void loop() {
  ensureMqttConnection();
 
  if (bno08x.wasReset()) {
    Serial.println("BNO08x reset detected, re-enabling reports");
    setBnoReports();
  }
 
  sh2_SensorValue_t sensorValue;
  bool gotLinear = false;
  while (bno08x.getSensorEvent(&sensorValue)) {
    updateImuFromEvent(sensorValue);
    if (sensorValue.sensorId == SH2_LINEAR_ACCELERATION) gotLinear = true;
  }
 
  const unsigned long now = millis();
 
  // Update vibration detection every time we get fresh linear accel data
  if (gotLinear) {
    updateVibration(now);
  }
 
  const bool shouldUpdate  = (now - lastSerialPrintTime)  >= SERIAL_PRINT_INTERVAL_MS;
  const bool shouldPublish = (now - lastMqttPublishTime)  >= MQTT_PUBLISH_INTERVAL_MS;
 
  if (!shouldUpdate && !shouldPublish) {
    delay(10);
    return;
  }
 
  const float temperatureC = bme.readTemperature();
  const float humidityPct  = bme.readHumidity();
  const float pressureHpa  = bme.readPressure() / 100.0f;
  const float altitudeM    = bme.readAltitude(SEA_LEVEL_PRESSURE_HPA);
 
  if (shouldUpdate) {
    lastSerialPrintTime = now;
    float rms = vibrationRms();
 
    Serial.printf(
        "%lu,%.2f,%.2f,%.2f,%.2f,"
        "%.3f,%.3f,%.3f,"
        "%.3f,%.3f,%.3f,"
        "%.3f,%.3f,%.3f,"
        "%.3f,%.3f,%.3f,"
        "%.2f,%.2f,%.2f,"
        "%.3f,%s\n",
        now,
        temperatureC, pressureHpa, humidityPct, altitudeM,
        imu.ax, imu.ay, imu.az,
        imu.gx, imu.gy, imu.gz,
        imu.lx, imu.ly, imu.lz,
        imu.gravx, imu.gravy, imu.gravz,
        imu.yaw, imu.pitch, imu.roll,
        rms,
        vibMaintenance ? "true" : "false");
  }
 
  if (shouldPublish) {
    lastMqttPublishTime = now;
    publishTelemetry(now, temperatureC, pressureHpa, humidityPct, altitudeM);
 
    if (!isnan(imu.roll)) {
      publishRoll(imu.roll);
    } else {
      Serial.println("imu.roll is NaN, skipping roll publish");
    }
  }
 
  delay(10);
}