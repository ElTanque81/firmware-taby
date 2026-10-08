// Vieri - animated face for Waveshare ESP32-S3-Touch-AMOLED-1.64 (V1.1 / "v2")
// Display init, opcodes and x-offset taken from Waveshare's official v2 demo.
// Board settings: ESP32S3 Dev Module, USB CDC On Boot = Enabled, Flash 16MB, PSRAM disabled.

#include <Arduino.h>
#include <Wire.h>
#include "lcd_bsp.h"
#include "freertos/semphr.h"

#define LCD_W 280
#define LCD_H 456
#define X_OFFSET 0x14   // panel is 280 wide inside a 320 wide controller RAM

#define PIN_TOUCH_SDA 47
#define PIN_TOUCH_SCL 48
#define TOUCH_ADDR 0x38

static esp_lcd_panel_handle_t panel = NULL;
static SemaphoreHandle_t txDone;

static bool onTxDone(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *e, void *ctx) {
  BaseType_t hp = pdFALSE;
  xSemaphoreGiveFromISR(txDone, &hp);
  return hp == pdTRUE;
}

// ---- drawing -------------------------------------------------------------
#define BAND_H 38                       // even, 456 = 12 * 38
static uint16_t *band;

// RGB565, byte-swapped (the panel wants big-endian pixel data)
static inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  return (c >> 8) | (c << 8);
}

static void pushBand(int y, int h) {
  esp_lcd_panel_draw_bitmap(panel, X_OFFSET, y, X_OFFSET + LCD_W, y + h, band);
  xSemaphoreTake(txDone, pdMS_TO_TICKS(200));   // wait until DMA is finished before reusing buffer
}

static void fillScreen(uint16_t c) {
  for (int i = 0; i < LCD_W * BAND_H; i++) band[i] = c;
  for (int y = 0; y < LCD_H; y += BAND_H) pushBand(y, BAND_H);
}

// rounded rectangle test (centre cx,cy, half sizes hw,hh, corner radius r)
static inline bool inRRect(int x, int y, int cx, int cy, int hw, int hh, int r) {
  int dx = abs(x - cx), dy = abs(y - cy);
  if (dx > hw || dy > hh) return false;
  dx -= (hw - r); dy -= (hh - r);
  if (dx <= 0 || dy <= 0) return true;
  return dx * dx + dy * dy <= r * r;
}

// ---- face state ----------------------------------------------------------
struct Face {
  float lookX = 0, lookY = 0;      // -1..1
  float tLookX = 0, tLookY = 0;
  float open = 1.0f;               // 1 = open, 0 = closed
  float happy = 0.0f;              // 0..1 squint-up
  float tHappy = 0.0f;
} face;

static const uint16_t BG = 0x0000;
static uint16_t EYE;

static void renderFace() {
  const int eyeY = 190, eyeDX = 62, hw = 38, fullH = 62;
  int hh = max(3, (int)(fullH * face.open * (1.0f - 0.45f * face.happy)));
  int ox = (int)(face.lookX * 22), oy = (int)(face.lookY * 28);
  int cxL = LCD_W / 2 - eyeDX + ox, cxR = LCD_W / 2 + eyeDX + ox, cy = eyeY + oy;
  int r = min(hw, hh) - 1;
  // mouth: small flat bar that curves up when happy
  int mouthY = 340, mouthW = 34 + (int)(30 * face.happy), mouthH = 5 + (int)(4 * face.happy);

  int yTop = 100, yBot = 400;       // only redraw the bands that can change
  for (int y0 = (yTop / BAND_H) * BAND_H; y0 < yBot; y0 += BAND_H) {
    for (int ry = 0; ry < BAND_H; ry++) {
      int y = y0 + ry;
      uint16_t *row = band + ry * LCD_W;
      for (int x = 0; x < LCD_W; x++) {
        bool on = inRRect(x, y, cxL, cy, hw, hh, r) || inRRect(x, y, cxR, cy, hw, hh, r) ||
                  inRRect(x, y, LCD_W / 2 + ox / 2, mouthY, mouthW, mouthH, mouthH);
        row[x] = on ? EYE : BG;
      }
    }
    pushBand(y0, BAND_H);
  }
}

// ---- touch ---------------------------------------------------------------
static bool readTouch(int &x, int &y) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(TOUCH_ADDR, 5) != 5) return false;
  uint8_t n = Wire.read();
  uint8_t b[4];
  for (int i = 0; i < 4; i++) b[i] = Wire.read();
  if (!n) return false;
  x = ((b[0] & 0x0F) << 8) | b[1];
  y = ((b[2] & 0x0F) << 8) | b[3];
  return true;
}

// ---- setup / loop --------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Vieri booting");

  txDone = xSemaphoreCreateBinary();

  panel = lcd_bsp_init(onTxDone, LCD_W * BAND_H * 2);

  band = (uint16_t *)heap_caps_malloc(LCD_W * BAND_H * 2, MALLOC_CAP_DMA);
  if (!band) { Serial.println("no DMA memory"); while (1) delay(1000); }

  EYE = rgb(120, 220, 255);

  // colour self-test: red, green, blue, then black. If you see these, the display works.
  fillScreen(rgb(255, 0, 0));   delay(400);
  fillScreen(rgb(0, 255, 0));   delay(400);
  fillScreen(rgb(0, 0, 255));   delay(400);
  fillScreen(BG);

  Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, 300000);
  Serial.println("Vieri ready");
}

void loop() {
  static uint32_t nextBlink = 2500, nextWander = 1500, lastTouch = 0, t0 = millis();
  static int blinkPhase = 0;                         // 0 none, 1 closing, 2 opening
  uint32_t t = millis() - t0;

  int tx, ty;
  if (readTouch(tx, ty)) {
    face.tLookX = constrain((tx - LCD_W / 2) / (LCD_W / 2.0f), -1.0f, 1.0f);
    face.tLookY = constrain((ty - LCD_H / 2) / (LCD_H / 2.0f), -1.0f, 1.0f);
    face.tHappy = 1.0f;
    lastTouch = t;
  } else if (t - lastTouch > 1200) {
    face.tHappy = 0.0f;
    if (t > nextWander) {                            // idle: look around randomly
      face.tLookX = random(-100, 101) / 100.0f;
      face.tLookY = random(-50, 51) / 100.0f;
      nextWander = t + random(1200, 3500);
    }
  }

  if (blinkPhase == 0 && t > nextBlink) blinkPhase = 1;
  if (blinkPhase == 1) { face.open -= 0.35f; if (face.open <= 0) { face.open = 0; blinkPhase = 2; } }
  else if (blinkPhase == 2) { face.open += 0.35f; if (face.open >= 1) { face.open = 1; blinkPhase = 0; nextBlink = t + random(2000, 5000); } }

  face.lookX += (face.tLookX - face.lookX) * 0.25f;
  face.lookY += (face.tLookY - face.lookY) * 0.25f;
  face.happy += (face.tHappy - face.happy) * 0.25f;

  renderFace();
}
