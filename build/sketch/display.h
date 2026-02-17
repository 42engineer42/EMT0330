#line 1 "C:\\Users\\jaanr\\Documents\\GitHub\\EMT0330\\Binary_clock\\display.h"
#pragma once

#include "config.h"
#include <Arduino.h>

void displayInit();
void displayUpdate(const ClockTime& now);
