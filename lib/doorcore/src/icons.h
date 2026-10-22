#pragma once
#include <Adafruit_GFX.h>

namespace door {

// GxEPD2 and GFXcanvas1 both treat non-zero as "white" (paper colour).
constexpr uint16_t kBlack = 0x0000;
constexpr uint16_t kWhite = 0xFFFF;

void drawThickLine(Adafruit_GFX& g, int x0, int y0, int x1, int y1, int width,
                   uint16_t color);

void drawUmbrella(Adafruit_GFX& g, int cx, int cy, int r, uint16_t color,
                  uint16_t background);
void drawSun(Adafruit_GFX& g, int cx, int cy, int r, uint16_t color);
void drawBatteryIcon(Adafruit_GFX& g, int x, int y, int w, int h, int percent,
                     uint16_t color);
void drawDegree(Adafruit_GFX& g, int cx, int cy, int r, int thickness,
                uint16_t color, uint16_t background);

}  // namespace door
