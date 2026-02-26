#include "clock.h"

namespace {
ClockTime currentStart = kDefaultStart;
unsigned long referenceMillis = 0;  // millis reference for currentStart
bool countForward = true;           // true: time moves forward, false: backward

constexpr size_t kCommandBufSize = 32;
char commandBuffer[kCommandBufSize];
size_t commandIndex = 0;
unsigned long lastCharMillis = 0;     // last serial char time (for timeout flush)

void printPrompt() {
  Serial.println(F("Binary clock ready. Send START=HH:MM:SS to set the initial time."));
  Serial.println(F("For example: START=21:30:00"));
  Serial.println(F("Change direction with DIR=UP or DIR=DOWN (default is UP)."));
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

bool equalsIgnoreCase(const char* a, const char* b) {
  while (*a && *b) {
    char x = *a++;
    char y = *b++;
    if (x >= 'A' && x <= 'Z') {
      x += 'a' - 'A';
    }
    if (y >= 'A' && y <= 'Z') {
      y += 'a' - 'A';
    }
    if (x != y) {
      return false;
    }
  }
  return *a == '\0' && *b == '\0';
}

void applyDirectionChange(bool forward) {
  // Re-anchor the start time so the display doesn't jump when changing direction.
  ClockTime snapshot = clockCurrent();
  currentStart = snapshot;
  referenceMillis = millis();
  countForward = forward;
  Serial.print(F("Direction set to "));
  Serial.println(forward ? F("UP") : F("DOWN"));
}
}  // namespace

void clockInit() {
  referenceMillis = millis();  // reset zero-point
  printPrompt();
}

void clockToggleDirection() {
  // Freeze the display at the current moment, flip direction, then continue from there.
  ClockTime snapshot = clockCurrent();
  currentStart = snapshot;
  referenceMillis = millis();
  countForward = !countForward;
  Serial.print(F("Direction set to "));
  Serial.println(countForward ? F("UP") : F("DOWN"));
}

ClockTime clockCurrent() {
  unsigned long elapsedSeconds = (millis() - referenceMillis) / 1000UL;
  long signedElapsed = countForward ? static_cast<long>(elapsedSeconds)
                                    : -static_cast<long>(elapsedSeconds);
  long totalSeconds = static_cast<long>(startSeconds()) + signedElapsed;
  long wrapped = ((totalSeconds % 86400L) + 86400L) % 86400L;  // keep positive
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
  // Change direction:
  // "DIR=UP" or "DIR=DOWN"
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
      if (startsWithIgnoreCase(commandBuffer, "DIR=")) {
        const char* dir = commandBuffer + 4;
        if (equalsIgnoreCase(dir, "UP") || equalsIgnoreCase(dir, "FORWARD") ||
            equalsIgnoreCase(dir, "+")) {
          applyDirectionChange(true);
        } else if (equalsIgnoreCase(dir, "DOWN") || equalsIgnoreCase(dir, "BACKWARD") ||
                   equalsIgnoreCase(dir, "-")) {
          applyDirectionChange(false);
        } else {
          Serial.println(F("Invalid direction. Use DIR=UP or DIR=DOWN."));
        }
        commandIndex = 0;
        continue;
      }
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
    if (startsWithIgnoreCase(commandBuffer, "DIR=")) {
      const char* dir = commandBuffer + 4;
      if (equalsIgnoreCase(dir, "UP") || equalsIgnoreCase(dir, "FORWARD") ||
          equalsIgnoreCase(dir, "+")) {
        applyDirectionChange(true);
      } else if (equalsIgnoreCase(dir, "DOWN") || equalsIgnoreCase(dir, "BACKWARD") ||
                 equalsIgnoreCase(dir, "-")) {
        applyDirectionChange(false);
      } else {
        Serial.println(F("Invalid direction. Use DIR=UP or DIR=DOWN."));
      }
      commandIndex = 0;
      return;
    }
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
