#pragma once
#include <Adafruit_GFX.h>

#include "attire.h"

namespace door {

// GxEPD2 and GFXcanvas1 both treat non-zero as "white" (paper colour).
constexpr uint16_t kBlack = 0x0000;
constexpr uint16_t kWhite = 0xFFFF;

enum class Fill : uint8_t { White, Black, Gray, LightGray };

struct Pt {
  int16_t x, y;
};

void fillPolygon(Adafruit_GFX& g, const Pt* pts, int n, Fill fill);
void strokePolygon(Adafruit_GFX& g, const Pt* pts, int n, uint16_t color);
void drawThickLine(Adafruit_GFX& g, int x0, int y0, int x1, int y1, int width,
                   uint16_t color);

// Person figure drawn in a 100 x 200 unit box, scaled by `scale`, with its
// top-left corner at (x, y).
constexpr int kPersonUnitsW = 100;
constexpr int kPersonUnitsH = 200;
void drawPerson(Adafruit_GFX& g, int x, int y, float scale, Gender gender,
                const Outfit& outfit);

void drawUmbrella(Adafruit_GFX& g, int cx, int cy, int r, uint16_t color,
                  uint16_t background);
void drawSun(Adafruit_GFX& g, int cx, int cy, int r, uint16_t color);
void drawBatteryIcon(Adafruit_GFX& g, int x, int y, int w, int h, int percent,
                     uint16_t color);
void drawDegree(Adafruit_GFX& g, int cx, int cy, int r, int thickness,
                uint16_t color, uint16_t background);

}  // namespace door
