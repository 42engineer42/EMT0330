#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/*
  ESP32-S3 WS2812/NeoPixel tester

  What this does:
  - tries a few common ESP32-S3 data pins
  - tries both GRB and RGB byte orders
  - shows RED, GREEN, BLUE on each attempt

  Typical onboard RGB LED pins on ESP32-S3 boards:
  - GPIO 48 is very common
  - GPIO 38 is used on some boards

  Wiring for an external WS2812:
  - LED 5V/VCC  -> board 5V or external 5V
  - LED GND     -> board GND
  - LED DIN     -> selected GPIO below
  - grounds must be shared
*/

static constexpr uint8_t kPinsToTry[] = {48, 38, 47, 21, 18};
static constexpr uint16_t kNumLeds = 1;
static constexpr uint8_t kBrightness = 32;
static constexpr uint32_t kStepMs = 1500;

struct PixelTypeOption {
  neoPixelType type;
  const char* name;
};

static const PixelTypeOption kPixelTypes[] = {
  {NEO_GRB + NEO_KHZ800, "GRB 800kHz"},
  {NEO_RGB + NEO_KHZ800, "RGB 800kHz"},
};

void showColour(Adafruit_NeoPixel& strip, uint8_t r, uint8_t g, uint8_t b, const char* name) {
  Serial.print("    colour: ");
  Serial.println(name);
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
  delay(kStepMs);
}

void runSingleTest(uint8_t pin, neoPixelType pixelType, const char* pixelTypeName) {
  Serial.println("--------------------------------------------------");
  Serial.print("Testing pin ");
  Serial.print(pin);
  Serial.print(" with ");
  Serial.println(pixelTypeName);

  Adafruit_NeoPixel strip(kNumLeds, pin, pixelType);
  strip.begin();
  strip.setBrightness(kBrightness);
  strip.clear();
  strip.show();
  delay(50);

  showColour(strip, 255, 0, 0, "RED");
  showColour(strip, 0, 255, 0, "GREEN");
  showColour(strip, 0, 0, 255, "BLUE");

  strip.clear();
  strip.show();
  delay(250);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("ESP32-S3 WS2812 tester");
  Serial.println("If the LED lights on one test, note the pin and pixel type.");
  Serial.println("If nothing lights at all, the problem is probably wiring or power.");
  Serial.println();
  Serial.println("External WS2812 checks:");
  Serial.println("  1. LED VCC must have 5V.");
  Serial.println("  2. LED GND and ESP32 GND must be connected together.");
  Serial.println("  3. Data must go to DIN, not DOUT.");
  Serial.println("  4. Many ESP32-S3 boards use GPIO 48 for onboard RGB LEDs.");
  Serial.println();

  for (const auto& option : kPixelTypes) {
    for (uint8_t pin : kPinsToTry) {
      runSingleTest(pin, option.type, option.name);
    }
  }

  Serial.println("Finished. If one combination worked, use that pin and pixel type in your real sketch.");
}

void loop() {}
