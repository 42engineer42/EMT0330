#pragma once

#include <Bluepad32.h>

// Initialize Bluetooth gamepad handling.
void xboxInit();

// Poll controller state; call this regularly from loop().
void xboxUpdate();
