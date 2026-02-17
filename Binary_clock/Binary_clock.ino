#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdio.h>

// TANK robot binary clock for a 128x64 monochrome OLED display.
// The clock is not tied to a RTC: it runs from the start time you set and keeps
// counting via millis(). Send "START=HH:MM:SS" (or just "HH:MM:SS") over Serial to
// change the starting point at any time.

constexpr int kOledWidth = 128;
constexpr int kOledHeight = 64;
constexpr int kOledAddress = 0x3C;
constexpr int kOledResetPin = -1;

Adafruit_SSD1306 display(kOledWidth, kOledHeight, &Wire, kOledResetPin);

struct ClockTime {
 uint8_t hour;
 uint8_t minute;
 uint8_t second;
};

constexpr ClockTime kDefaultStart = {12, 0, 0};
ClockTime currentStart = kDefaultStart;
unsigned long referenceMillis = 0;
unsigned long lastLogMillis = 0;

constexpr size_t kCommandBufSize = 32;
char commandBuffer[kCommandBufSize];
size_t commandIndex = 0;

void printPrompt() {
  Serial.println(F("Binary clock ready. Send START=HH:MM:SS to set the initial time."));
  Serial.println(F("For example: START=21:30:00"));
}

void formatTimeString(const ClockTime& time, char* buffer, size_t bufferSize) {
  snprintf(buffer, bufferSize, "%02u:%02u:%02u", time.hour, time.minute, time.second);
}

bool tryParseTime(const char* text, ClockTime& out) {
  int h = -1;
  int m = -1;
  int s = -1;
  if (sscanf(text, "%d:%d:%d", &h, &m, &s) != 3) {
    return false;
  }
 if (h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59) {
 return false;
 }
 out.hour = h;
 out.minute = m;
 out.second = s;
 return true;
}

bool startsWithIgnoreCase(const char* text, const char* prefix) {
 while (*prefix) {
 char a = *text++;
 char b = *prefix++;
 if (a == '\0') {
 return false;
 }
 if (a >= 'A' && a <= 'Z') {
 a += 'a' - 'A';
 }
 if (b >= 'A' && b <= 'Z') {
 b += 'a' - 'A';
 }
 if (a != b) {
 return false;
 }
 }
 return true;
}

void setStartTime(const ClockTime& newStart) {
  currentStart = newStart;
  referenceMillis = millis();
  char buf[16];
  formatTimeString(currentStart, buf, sizeof(buf));
  Serial.print(F("Start time set to "));
  Serial.println(buf);
}

void processCommand(const char* rawInput) {
 char trimmed[kCommandBufSize];
 size_t outIndex = 0;
 for (size_t i = 0; rawInput[i] != '\0' && outIndex < kCommandBufSize - 1; ++i) {
 if (rawInput[i] == ' ' || rawInput[i] == '\t') {
 continue;
 }
 trimmed[outIndex++] = rawInput[i];
 }
 trimmed[outIndex] = '\0';
 if (outIndex == 0) {
 return;
 }
 const char* probe = trimmed;
  if (startsWithIgnoreCase(trimmed, "START=")) {
 probe = trimmed + 6;
 }
 ClockTime requested;
 if (tryParseTime(probe, requested)) {
 setStartTime(requested);
 } else {
    Serial.println(F("Invalid format. Use HH:MM:SS or START=HH:MM:SS"));
 }
}

void handleSerialInput() {
  while (Serial.available()) {
    char incoming = Serial.read();
    if (incoming == '\r') {
      continue;
    }
    if (incoming == '\n') {
      commandBuffer[commandIndex] = '\0';
      processCommand(commandBuffer);
      commandIndex = 0;
      continue;
    }
    if (commandIndex < kCommandBufSize - 1) {
      commandBuffer[commandIndex++] = incoming;
    } else {
      commandIndex = 0;
    }
  }
}

unsigned long startSeconds() {
 return static_cast<unsigned long>(currentStart.hour) * 3600UL +
 static_cast<unsigned long>(currentStart.minute) * 60UL +
 currentStart.second;
}

ClockTime computeCurrentTime() {
 unsigned long elapsedSeconds = (millis() - referenceMillis) / 1000UL;
 unsigned long totalSeconds = startSeconds() + elapsedSeconds;
 unsigned long wrapped = totalSeconds % 86400UL;
 ClockTime now;
 now.hour = (wrapped / 3600UL) % 24;
 now.minute = (wrapped / 60UL) % 60;
 now.second = wrapped % 60;
 return now;
}

void formatBinary(uint8_t value, uint8_t bits, char* output) {
 for (uint8_t bit = 0; bit < bits; ++bit) {
 uint8_t shift = bits - 1 - bit;
 output[bit] = (value >> shift) & 1 ? '1' : '0';
 }
 output[bits] = '\0';
}

void drawBinaryRow(const char* label, uint8_t value, uint8_t bits, uint8_t y) {
 char binary[9];
 formatBinary(value, bits, binary);
 display.setCursor(0, y);
 display.print(label);
  display.print(' ');
 display.print(binary);
}

void drawDisplay(const ClockTime& now) {
 char timeText[16];
 formatTimeString(now, timeText, sizeof(timeText));

 display.clearDisplay();
 display.setTextColor(SSD1306_WHITE);
 display.setTextSize(2);
 display.setCursor(0, 0);
 display.println(timeText);

 display.setTextSize(1);
  drawBinaryRow("H", now.hour, 6, 32);
  drawBinaryRow("M", now.minute, 6, 44);
  drawBinaryRow("S", now.second, 6, 56);

 display.display();
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("donkey");
  // Use custom I2C pins (SDA=34, SCL=48) for this ESP32 variant.
  Wire.begin(34, 48);
  if (!display.begin(SSD1306_SWITCHCAPVCC, kOledAddress)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true) {
    delay(1000);
  }
 }
 display.clearDisplay();
 display.setTextColor(SSD1306_WHITE);
 referenceMillis = millis();
 printPrompt();
}

void loop() {
  handleSerialInput();
  ClockTime now = computeCurrentTime();
  drawDisplay(now);
  if (millis() - lastLogMillis >= 1000UL) {
    char buf[16];
    formatTimeString(now, buf, sizeof(buf));
    Serial.print(F("Time: "));
    Serial.println(buf);
    lastLogMillis = millis();
  }
  delay(100);
}
