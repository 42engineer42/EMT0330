#line 1 "C:\\Users\\jaanr\\Documents\\GitHub\\EMT0330\\Binary_clock\\clock.h"
#pragma once

#include "config.h"
#include <Arduino.h>

// Initializes clock state and prints the prompt.
void clockInit();

// Handle incoming serial commands (START=HH:MM:SS).
void clockHandleSerial();

// Compute the current time based on millis() since reference start.
ClockTime clockCurrent();
