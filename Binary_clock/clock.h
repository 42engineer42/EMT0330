#pragma once

#include "config.h"
#include <Arduino.h>

// Initializes clock state and prints the prompt.
void clockInit();

// Handle incoming serial commands (START=HH:MM:SS).
void clockHandleSerial();

// Compute the current time based on millis() since reference start.
ClockTime clockCurrent();

// Toggle counting direction (forward/backward) and keep time continuous.
void clockToggleDirection();
