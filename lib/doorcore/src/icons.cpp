#include "icons.h"

#include <initializer_list>
#include <math.h>

namespace door {

void drawThickLine(Adafruit_GFX& g, int x0, int y0, int x1, int y1, int w,
                   uint16_t color) {
  if (w <= 1) {
    g.drawLine(x0, y0, x1, y1, color);
    return;
  }
  float dx = static_cast<float>(x1 - x0), dy = static_cast<float>(y1 - y0);
  float len = sqrtf(dx * dx + dy * dy);
  if (len < 0.5f) {
    g.fillCircle(x0, y0, w / 2, color);
    return;
  }
  float nx = -dy / len * w / 2.0f, ny = dx / len * w / 2.0f;
  const int ax = lroundf(x0 + nx), ay = lroundf(y0 + ny);
  const int bx = lroundf(x1 + nx), by = lroundf(y1 + ny);
  const int cx = lroundf(x1 - nx), cy = lroundf(y1 - ny);
  const int dx2 = lroundf(x0 - nx), dy2 = lroundf(y0 - ny);
  g.fillTriangle(ax, ay, bx, by, cx, cy, color);
  g.fillTriangle(ax, ay, cx, cy, dx2, dy2, color);
  g.fillCircle(x0, y0, (w - 1) / 2, color);
  g.fillCircle(x1, y1, (w - 1) / 2, color);
}

void drawUmbrella(Adafruit_GFX& g, int cx, int cy, int r, uint16_t c, uint16_t bg) {
  // Canopy: half disc with a scalloped lower edge.
  g.fillCircle(cx, cy, r, c);
  g.fillRect(cx - r - 1, cy + 1, 2 * r + 3, r + 1, bg);
  int s = r / 3;
  for (int i = -1; i <= 1; ++i) g.fillCircle(cx + i * 2 * s, cy + 1, s, bg);
  g.fillCircle(cx, cy - r, r / 12 + 1, c);  // tip
  // Shaft and J handle.
  int w = r / 9 + 2;
  int bottom = cy + r;
  g.fillRect(cx - w / 2, cy - s / 2, w, bottom - cy + s / 2, c);
  int hr = r / 5;
  for (int t = 0; t < w; ++t) g.drawCircleHelper(cx - hr, bottom, hr + w / 2 - t, 0x4 | 0x8, c);
}

void drawSun(Adafruit_GFX& g, int cx, int cy, int r, uint16_t c) {
  int core = r * 5 / 10;
  g.fillCircle(cx, cy, core, c);
  for (int i = 0; i < 8; ++i) {
    float a = i * 3.14159265f / 4;
    int x0 = cx + static_cast<int>(cosf(a) * (core + r / 6));
    int y0 = cy + static_cast<int>(sinf(a) * (core + r / 6));
    int x1 = cx + static_cast<int>(cosf(a) * r);
    int y1 = cy + static_cast<int>(sinf(a) * r);
    drawThickLine(g, x0, y0, x1, y1, r / 8 + 2, c);
  }
}

void drawBatteryIcon(Adafruit_GFX& g, int x, int y, int w, int h, int pct, uint16_t c) {
  int tip = w / 10 + 1;
  int bodyW = w - tip;
  g.drawRect(x, y, bodyW, h, c);
  g.drawRect(x + 1, y + 1, bodyW - 2, h - 2, c);
  g.fillRect(x + bodyW, y + h / 4, tip, h - 2 * (h / 4), c);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  int inner = bodyW - 8;
  int fill = (inner * pct + 50) / 100;
  if (pct > 0 && fill < 2) fill = 2;
  if (fill > 0) g.fillRect(x + 4, y + 4, fill, h - 8, c);
}

void drawDegree(Adafruit_GFX& g, int cx, int cy, int r, int t, uint16_t c, uint16_t bg) {
  g.fillCircle(cx, cy, r, c);
  g.fillCircle(cx, cy, r - t, bg);
}

}  // namespace door
