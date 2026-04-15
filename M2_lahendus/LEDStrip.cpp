#include "LEDStrip.h"
#include <Arduino.h>

#define DIST_MIN 30
#define DIST_MAX 2000

LEDStrip::LEDStrip(uint8_t pin, uint16_t numPixels)
    : strip(numPixels, pin, NEO_GRB + NEO_KHZ800), numPixels(numPixels)
{}

void LEDStrip::begin() {
    strip.begin();
    strip.show();
}

void LEDStrip::setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= numPixels) return;
    strip.setPixelColor(index, strip.Color(r, g, b));
}

void LEDStrip::clear() {
    strip.clear();
}

void LEDStrip::show() {
    strip.show();
}

// Convert distance to a color between Green (far) and Red (near)
uint32_t LEDStrip::colorFromDistance(uint16_t dist) {
    dist = constrain(dist, DIST_MIN, DIST_MAX);

    float ratio = float(dist - DIST_MIN) / float(DIST_MAX - DIST_MIN);

    // near = red (255,0,0)
    // far = green (0,255,0)
    uint8_t r = 25 * (1.0f - ratio);
    uint8_t g = 25 * ratio;

    return strip.Color(r, g, 0);
}

// ASSIGNMENT:
// LED index layout (0 = RIGHT → 6 = LEFT)
// Right group: 0,1
// Middle group: 2,3,4
// Left group: 5,6
void LEDStrip::setGroupColorsByDistance(uint16_t distRight, uint16_t distMid, uint16_t distLeft) {
    uint32_t colRight = colorFromDistance(distRight);
    uint32_t colMid   = colorFromDistance(distMid);
    uint32_t colLeft  = colorFromDistance(distLeft);

    // Right 2 LEDs
    strip.setPixelColor(0, colRight);
    strip.setPixelColor(1, colRight);

    // Middle 3 LEDs
    strip.setPixelColor(2, colMid);
    strip.setPixelColor(3, colMid);
    strip.setPixelColor(4, colMid);

    // Left 2 LEDs
    strip.setPixelColor(5, colLeft);
    strip.setPixelColor(6, colLeft);

    strip.show();
}