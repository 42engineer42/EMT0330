#include "clock.h"

namespace {
ClockTime currentStart = kDefaultStart;
unsigned long referenceMillis = 0;

constexpr size_t kCommandBufSize = 32;
char commandBuffer[kCommandBufSize];
size_t commandIndex = 0;
unsigned long lastCharMillis = 0;

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

unsigned long startSeconds() {
  return static_cast<unsigned long>(currentStart.hour) * 3600UL +
         static_cast<unsigned long>(currentStart.minute) * 60UL +
         currentStart.second;
}
}  // namespace

void clockInit() {
  referenceMillis = millis();
  printPrompt();
}

ClockTime clockCurrent() {
  unsigned long elapsedSeconds = (millis() - referenceMillis) / 1000UL;
  unsigned long totalSeconds = startSeconds() + elapsedSeconds;
  unsigned long wrapped = totalSeconds % 86400UL;
  ClockTime now;
  now.hour = (wrapped / 3600UL) % 24;
  now.minute = (wrapped / 60UL) % 60;
  now.second = wrapped % 60;
  return now;
}

void clockHandleSerial() {
  // Change the clock time by sending either:
  // "HH:MM:SS"            (e.g., 12:34:56)
  // "START=HH:MM:SS"      (e.g., START=07:00:00)
  // Lines end with newline; hours 0-23, minutes/seconds 0-59.
  while (Serial.available()) {
    char incoming = Serial.read();
    lastCharMillis = millis();
    if (incoming == '\r') {
      incoming = '\n';  // treat CR as newline too
    }
    if (incoming == '\n') {
      commandBuffer[commandIndex] = '\0';
      const char* probe = commandBuffer;
      if (startsWithIgnoreCase(commandBuffer, "START=")) {
        probe = commandBuffer + 6;
      }
      ClockTime requested;
      if (tryParseTime(probe, requested)) {
        currentStart = requested;
        referenceMillis = millis();
        char buf[16];
        formatTimeString(currentStart, buf, sizeof(buf));
        Serial.print(F("Start time set to "));
        Serial.println(buf);
      } else {
        Serial.println(F("Invalid format. Use HH:MM:SS or START=HH:MM:SS"));
      }
      commandIndex = 0;
      continue;
    }
    if (commandIndex < kCommandBufSize - 1) {
      commandBuffer[commandIndex++] = incoming;
    } else {
      commandIndex = 0;
    }
  }
  // Fallback: if no newline received but input paused, process buffered line.
  if (commandIndex > 0 && (millis() - lastCharMillis) > 250) {
    commandBuffer[commandIndex] = '\0';
    const char* probe = commandBuffer;
    if (startsWithIgnoreCase(commandBuffer, "START=")) {
      probe = commandBuffer + 6;
    }
    ClockTime requested;
    if (tryParseTime(probe, requested)) {
      currentStart = requested;
      referenceMillis = millis();
      char buf[16];
      formatTimeString(currentStart, buf, sizeof(buf));
      Serial.print(F("Start time set to "));
      Serial.println(buf);
    } else {
      Serial.println(F("Invalid format. Use HH:MM:SS or START=HH:MM:SS"));
    }
    commandIndex = 0;
  }
}
