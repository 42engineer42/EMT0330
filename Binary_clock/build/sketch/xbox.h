#line 1 "C:\\Users\\jaanr\\Documents\\GitHub\\EMT0330\\Binary_clock\\xbox.h"
#pragma once

#include <Bluepad32.h>

// Initialize Bluetooth gamepad handling.
void xboxInit();

// Poll controller state; call this regularly from loop().
void xboxUpdate();
