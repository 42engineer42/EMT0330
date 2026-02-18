#line 1 "C:\\Users\\jaanr\\Documents\\GitHub\\EMT0330\\Binary_clock\\display.cpp"
#include "display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

namespace {
Adafruit_SSD1306 display(kOledWidth, kOledHeight, &Wire, kOledResetPin);

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
}  // namespace

void displayInit() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, kOledAddress)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true) {
      delay(1000);
    }
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
}

void displayUpdate(const ClockTime& now) {
  char timeText[16];
  snprintf(timeText, sizeof(timeText), "%02u:%02u:%02u", now.hour, now.minute, now.second);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(timeText);

  display.setTextSize(1);
  drawBinaryRow("H", now.hour, 6, 32);
  drawBinaryRow("M", now.minute, 6, 44);
  drawBinaryRow("S", now.second, 6, 56);

  display.display();
}
