/*
  ESP32-S3 RGB LED auto-tester — pin 38
  Opens Serial at 115200. Watch the output to see which test lights the LED.
  Each test runs RED → GREEN → BLUE (2 s each) then moves on.
*/

#include <Arduino.h>

// ─── shared settings ─────────────────────────────────────────────────────────
#define LED_PIN     38
#define NUM_LEDS    1
#define STEP_MS     2000   // ms per colour per test

// ─── Test A: FastLED ─────────────────────────────────────────────────────────
#include <FastLED.h>
CRGB leds_fastled[NUM_LEDS];

void testFastLED() {
  Serial.println("==============================================");
  Serial.println("TEST A — FastLED  (WS2812B / NEOPIXEL type)");
  Serial.println("==============================================");

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds_fastled, NUM_LEDS);
  FastLED.setBrightness(80);

  const char* colours[] = {"RED", "GREEN", "BLUE"};
  CRGB vals[]           = {CRGB::Red, CRGB::Green, CRGB::Blue};

  for (int i = 0; i < 3; i++) {
    Serial.print("  FastLED -> "); Serial.println(colours[i]);
    leds_fastled[0] = vals[i];
    FastLED.show();
    delay(STEP_MS);
  }

  // turn off before next test
  leds_fastled[0] = CRGB::Black;
  FastLED.show();
  delay(300);
}

// ─── Test B: Adafruit NeoPixel ───────────────────────────────────────────────
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void testAdafruitNeoPixel() {
  Serial.println("==============================================");
  Serial.println("TEST B — Adafruit NeoPixel  (NEO_GRB 800 kHz)");
  Serial.println("==============================================");

  strip.begin();
  strip.setBrightness(80);

  uint32_t colours[] = {strip.Color(255,0,0), strip.Color(0,255,0), strip.Color(0,0,255)};
  const char* names[]= {"RED","GREEN","BLUE"};

  for (int i = 0; i < 3; i++) {
    Serial.print("  Adafruit -> "); Serial.println(names[i]);
    strip.setPixelColor(0, colours[i]);
    strip.show();
    delay(STEP_MS);
  }

  strip.setPixelColor(0, 0);
  strip.show();
  delay(300);
}

// ─── Test C: NeoPixelBus (hardware RMT on ESP32-S3) ─────────────────────────
#include <NeoPixelBus.h>

// RmtN = RMT channel N (0-based). Channel 0 is usually free.
NeoPixelBus<NeoGrbFeature, NeoEsp32RmtNWs2812xMethod> bus(NUM_LEDS, LED_PIN);

void testNeoPixelBus() {
  Serial.println("==============================================");
  Serial.println("TEST C — NeoPixelBus  (NeoGRB + ESP32 RMT ch0)");
  Serial.println("==============================================");

  bus.Begin();
  bus.ClearTo(RgbColor(0));
  bus.Show();

  RgbColor colours[] = {RgbColor(80,0,0), RgbColor(0,80,0), RgbColor(0,0,80)};
  const char* names[]= {"RED","GREEN","BLUE"};

  for (int i = 0; i < 3; i++) {
    Serial.print("  NeoPixelBus -> "); Serial.println(names[i]);
    bus.SetPixelColor(0, colours[i]);
    bus.Show();
    delay(STEP_MS);
  }

  bus.ClearTo(RgbColor(0));
  bus.Show();
  delay(300);
}

// ─── Test D: Raw ESP-IDF RMT (no extra library) ──────────────────────────────
#include <driver/rmt.h>

#define RMT_CHANNEL     RMT_CHANNEL_1
#define RMT_CLK_DIV     4          // 80 MHz / 4 = 20 MHz → 50 ns per tick
// WS2812 timings in 50 ns ticks
#define T0H  8   // 0-bit high  ~400 ns
#define T0L  17  // 0-bit low   ~850 ns
#define T1H  16  // 1-bit high  ~800 ns
#define T1L  9   // 1-bit low   ~450 ns

void sendRmtPixel(uint8_t r, uint8_t g, uint8_t b) {
  // WS2812 sends GRB
  uint8_t bytes[3] = {g, r, b};
  rmt_item32_t items[24];
  int idx = 0;
  for (int byte_i = 0; byte_i < 3; byte_i++) {
    for (int bit_i = 7; bit_i >= 0; bit_i--) {
      bool bit = (bytes[byte_i] >> bit_i) & 1;
      items[idx].level0    = 1;
      items[idx].duration0 = bit ? T1H : T0H;
      items[idx].level1    = 0;
      items[idx].duration1 = bit ? T1L : T0L;
      idx++;
    }
  }
  rmt_write_items(RMT_CHANNEL, items, 24, true);
  rmt_wait_tx_done(RMT_CHANNEL, pdMS_TO_TICKS(100));
}

void testRawRMT() {
  Serial.println("==============================================");
  Serial.println("TEST D — Raw ESP-IDF RMT  (GRB, 800 kHz hand-coded)");
  Serial.println("==============================================");

  rmt_config_t cfg = {};
  cfg.channel       = RMT_CHANNEL;
  cfg.gpio_num      = (gpio_num_t)LED_PIN;
  cfg.mem_block_num = 1;
  cfg.clk_div       = RMT_CLK_DIV;
  cfg.tx_config.loop_en          = false;
  cfg.tx_config.carrier_en       = false;
  cfg.tx_config.idle_output_en   = true;
  cfg.tx_config.idle_level       = RMT_IDLE_LEVEL_LOW;
  cfg.rmt_mode = RMT_MODE_TX;
  rmt_config(&cfg);
  rmt_driver_install(RMT_CHANNEL, 0, 0);

  struct { uint8_t r, g, b; const char* name; } colours[] = {
    {80, 0,  0,  "RED"},
    {0,  80, 0,  "GREEN"},
    {0,  0,  80, "BLUE"},
  };

  for (auto& c : colours) {
    Serial.print("  Raw RMT -> "); Serial.println(c.name);
    sendRmtPixel(c.r, c.g, c.b);
    delay(STEP_MS);
  }

  sendRmtPixel(0, 0, 0);
  rmt_driver_uninstall(RMT_CHANNEL);
  delay(300);
}

// ─── Main ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1500);   // give Serial monitor time to connect
  Serial.println("\n\nESP32-S3 RGB LED tester — pin 38");
  Serial.println("Watch which test lights the LED correctly.\n");

  testFastLED();
  testAdafruitNeoPixel();
  testNeoPixelBus();
  testRawRMT();

  Serial.println("\nAll tests done. Check which test worked and tell me!");
}

void loop() {}