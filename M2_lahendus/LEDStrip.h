#ifndef LEDSTRIP_H
#define LEDSTRIP_H

#include <Adafruit_NeoPixel.h>

class LEDStrip {
public:
    LEDStrip(uint8_t pin, uint16_t numPixels);

    void begin();
    void setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    void clear();
    void show();

    void setGroupColorsByDistance(uint16_t distRight, uint16_t distMid, uint16_t distLeft);

private:
    uint32_t colorFromDistance(uint16_t dist);

    Adafruit_NeoPixel strip;
    uint16_t numPixels;
};

#endif
