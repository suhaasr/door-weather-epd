#include "figures.h"

#include <initializer_list>
#include <math.h>

namespace door {

namespace {

inline bool patternBlack(Fill fill, int x, int y) {
  switch (fill) {
    case Fill::Black: return true;
    case Fill::White: return false;
    case Fill::Gray: return ((x + y) & 1) == 0;
    case Fill::LightGray: return (x & 1) == 0 && (y & 1) == 0;
  }
  return false;
}

void fillSpan(Adafruit_GFX& g, int x0, int x1, int y, Fill fill) {
  if (x1 < x0) return;
  if (fill == Fill::Black || fill == Fill::White) {
    g.drawFastHLine(x0, y, x1 - x0 + 1, fill == Fill::Black ? kBlack : kWhite);
    return;
  }
  for (int x = x0; x <= x1; ++x) {
    g.drawPixel(x, y, patternBlack(fill, x, y) ? kBlack : kWhite);
  }
}

constexpr int kMaxPolyPoints = 16;

}  // namespace

void fillPolygon(Adafruit_GFX& g, const Pt* p, int n, Fill fill) {
  if (n < 3 || n > kMaxPolyPoints) return;
  int minY = p[0].y, maxY = p[0].y;
  for (int i = 1; i < n; ++i) {
    if (p[i].y < minY) minY = p[i].y;
    if (p[i].y > maxY) maxY = p[i].y;
  }
  float xs[kMaxPolyPoints];
  for (int y = minY; y <= maxY; ++y) {
    float sy = y + 0.5f;
    int count = 0;
    for (int i = 0, j = n - 1; i < n; j = i++) {
      float yi = p[i].y, yj = p[j].y;
      if ((yi <= sy && yj > sy) || (yj <= sy && yi > sy)) {
        xs[count++] = p[i].x + (sy - yi) * (p[j].x - p[i].x) / (yj - yi);
      }
    }
    for (int a = 1; a < count; ++a) {  // insertion sort, count is tiny
      float v = xs[a];
      int b = a - 1;
      while (b >= 0 && xs[b] > v) {
        xs[b + 1] = xs[b];
        --b;
      }
      xs[b + 1] = v;
    }
    for (int k = 0; k + 1 < count; k += 2) {
      fillSpan(g, static_cast<int>(lroundf(xs[k])),
               static_cast<int>(lroundf(xs[k + 1])) - 1, y, fill);
    }
  }
}

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
  Pt q[4] = {{(int16_t)lroundf(x0 + nx), (int16_t)lroundf(y0 + ny)},
             {(int16_t)lroundf(x1 + nx), (int16_t)lroundf(y1 + ny)},
             {(int16_t)lroundf(x1 - nx), (int16_t)lroundf(y1 - ny)},
             {(int16_t)lroundf(x0 - nx), (int16_t)lroundf(y0 - ny)}};
  fillPolygon(g, q, 4, color == kBlack ? Fill::Black : Fill::White);
  g.fillCircle(x0, y0, (w - 1) / 2, color);
  g.fillCircle(x1, y1, (w - 1) / 2, color);
}

void strokePolygon(Adafruit_GFX& g, const Pt* p, int n, uint16_t color) {
  for (int i = 0, j = n - 1; i < n; j = i++) {
    drawThickLine(g, p[j].x, p[j].y, p[i].x, p[i].y, 2, color);
  }
}

namespace {

struct UPt {
  float x, y;
};

// Maps figure units to pixels and offers the few shapes the figures use.
class Pen {
 public:
  Pen(Adafruit_GFX& g, int ox, int oy, float s) : g_(g), ox_(ox), oy_(oy), s_(s) {}

  int16_t X(float x) const { return static_cast<int16_t>(lroundf(ox_ + x * s_)); }
  int16_t Y(float y) const { return static_cast<int16_t>(lroundf(oy_ + y * s_)); }
  int R(float r) const { return r * s_ < 1 ? 1 : static_cast<int>(lroundf(r * s_)); }

  // Black shapes get a thin white halo so they stay distinct from black
  // shapes underneath (e.g. a black sleeve over a black coat).
  void poly(std::initializer_list<UPt> pts, Fill fill) {
    Pt p[kMaxPolyPoints];
    int n = 0;
    for (const UPt& u : pts) {
      if (n == kMaxPolyPoints) break;
      p[n++] = {X(u.x), Y(u.y)};
    }
    if (fill == Fill::Black) strokePolygon(g_, p, n, kWhite);
    fillPolygon(g_, p, n, fill);
    if (fill != Fill::Black) strokePolygon(g_, p, n, kBlack);
  }

  // A limb segment from (x0,y0) to (x1,y1) with the given width.
  void limb(float x0, float y0, float x1, float y1, float w, Fill fill) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    float nx = -dy / len * w / 2, ny = dx / len * w / 2;
    poly({{x0 + nx, y0 + ny}, {x1 + nx, y1 + ny}, {x1 - nx, y1 - ny}, {x0 - nx, y0 - ny}},
         fill);
  }

  void circle(float cx, float cy, float r, Fill fill) {
    int px = X(cx), py = Y(cy), pr = R(r);
    if (fill == Fill::Black) {
      g_.fillCircle(px, py, pr + 1, kWhite);
      g_.fillCircle(px, py, pr, kBlack);
    } else {
      g_.fillCircle(px, py, pr, kWhite);
      g_.drawCircle(px, py, pr, kBlack);
      g_.drawCircle(px, py, pr - 1, kBlack);
    }
  }

  void dot(float cx, float cy, float r, uint16_t color) {
    g_.fillCircle(X(cx), Y(cy), R(r), color);
  }

  void line(float x0, float y0, float x1, float y1, int w, uint16_t color) {
    drawThickLine(g_, X(x0), Y(y0), X(x1), Y(y1), w, color);
  }

  Adafruit_GFX& gfx() { return g_; }

 private:
  Adafruit_GFX& g_;
  int ox_, oy_;
  float s_;
};

constexpr float kShoulderY = 48, kHandY = 114, kWaistY = 104, kFootY = 186;

void drawLegs(Pen& p) {
  p.poly({{34, 104}, {49, 104}, {47, kFootY}, {37, kFootY}}, Fill::White);
  p.poly({{51, 104}, {66, 104}, {63, kFootY}, {53, kFootY}}, Fill::White);
}

void drawBottom(Pen& p, Bottom b) {
  switch (b) {
    case Bottom::Pants:
      p.poly({{33, 100}, {50, 100}, {48, kFootY + 2}, {35, kFootY + 2}}, Fill::Gray);
      p.poly({{50, 100}, {67, 100}, {65, kFootY + 2}, {52, kFootY + 2}}, Fill::Gray);
      break;
    case Bottom::Shorts:
      p.poly({{32, 100}, {50, 100}, {49, 136}, {33, 136}}, Fill::Gray);
      p.poly({{50, 100}, {68, 100}, {67, 136}, {51, 136}}, Fill::Gray);
      break;
    case Bottom::Skirt:
      p.poly({{34, 98}, {66, 98}, {76, 146}, {24, 146}}, Fill::Black);
      break;
    case Bottom::None:
      break;
  }
}

void drawShoes(Pen& p, Shoes s) {
  for (int side = 0; side < 2; ++side) {
    float l = side == 0 ? 31 : 51;  // left edge of the foot
    float toeL = side == 0 ? l : l + 2, toeR = side == 0 ? l + 16 : l + 18;
    switch (s) {
      case Shoes::Sneakers:
        p.poly({{toeL + 2, 182}, {toeR - 2, 182}, {toeR, 189}, {toeR, 195}, {toeL, 195},
                {toeL, 189}},
               Fill::Black);
        p.line(toeL + 1, 192, toeR - 1, 192, 1, kWhite);
        break;
      case Shoes::Boots:
        p.poly({{toeL + 3, 160}, {toeR - 3, 160}, {toeR - 2, 186}, {toeR, 189}, {toeR, 195},
                {toeL, 195}, {toeL, 189}, {toeL + 3, 186}},
               Fill::Black);
        break;
      case Shoes::Sandals:
        p.poly({{toeL + 3, 184}, {toeR - 3, 184}, {toeR, 190}, {toeL, 190}}, Fill::White);
        p.poly({{toeL - 1, 190}, {toeR + 1, 190}, {toeR + 1, 195}, {toeL - 1, 195}}, Fill::Black);
        break;
    }
  }
}

void drawTorso(Pen& p, Gender g, Top top) {
  const bool man = g == Gender::Man;
  switch (top) {
    case Top::Tank:
      p.poly({{37, 44}, {63, 44}, {68, kWaistY + 2}, {32, kWaistY + 2}}, Fill::White);
      p.line(41, 44, 41, 40, 2, kBlack);
      p.line(59, 44, 59, 40, 2, kBlack);
      break;
    case Top::TShirt:
    case Top::LongSleeve:
      if (man) {
        p.poly({{29, 44}, {71, 44}, {68, kWaistY + 2}, {32, kWaistY + 2}}, Fill::White);
      } else {
        p.poly({{31, 44}, {69, 44}, {64, 76}, {67, kWaistY}, {33, kWaistY}, {36, 76}},
               Fill::White);
      }
      p.line(44, 44, 50, 50, 1, kBlack);  // collar
      p.line(50, 50, 56, 44, 1, kBlack);
      break;
    case Top::Sweater:
      if (man) {
        p.poly({{29, 44}, {71, 44}, {68, 108}, {32, 108}}, Fill::LightGray);
      } else {
        p.poly({{31, 44}, {69, 44}, {65, 78}, {68, 108}, {32, 108}, {35, 78}}, Fill::LightGray);
      }
      p.poly({{32, 101}, {68, 101}, {68, 108}, {32, 108}}, Fill::Black);  // ribbed hem
      p.poly({{42, 42}, {58, 42}, {58, 47}, {42, 47}}, Fill::Black);      // crew neck
      break;
    case Top::Jacket:
      p.poly({{28, 43}, {72, 43}, {71, 112}, {29, 112}}, Fill::Black);
      p.line(50, 50, 50, 111, 1, kWhite);  // zipper
      p.poly({{40, 41}, {50, 52}, {44, 56}, {36, 44}}, Fill::Black);  // collar
      p.poly({{60, 41}, {64, 44}, {56, 56}, {50, 52}}, Fill::Black);
      break;
    case Top::Coat:
    case Top::LongCoat: {
      float hem = top == Top::LongCoat ? 164 : 144;
      float flare = top == Top::LongCoat ? 9 : 3;
      p.poly({{27, 43}, {73, 43}, {72 + flare, hem}, {28 - flare, hem}}, Fill::Black);
      p.line(43, 44, 50, 64, 1, kWhite);  // lapels
      p.line(57, 44, 50, 64, 1, kWhite);
      p.line(50, 64, 50, hem - 1, 1, kWhite);
      for (float by = 76; by < hem - 8; by += 20) p.dot(55, by, 1.8f, kWhite);
      p.line(29, 112, 71, 112, 1, kWhite);  // belt line
      break;
    }
    case Top::Sundress:
      p.poly({{38, 44}, {62, 44}, {64, 80}, {78, 150}, {22, 150}, {36, 80}}, Fill::LightGray);
      p.line(36, 80, 64, 80, 2, kBlack);
      p.line(41, 44, 41, 39, 2, kBlack);
      p.line(59, 44, 59, 39, 2, kBlack);
      break;
  }
}

void drawArms(Pen& p, Top top, bool gloves) {
  const float sx[2] = {30, 70};
  const float hx[2] = {19.5f, 80.5f};
  Fill sleeveFill = Fill::White;
  float sleeveFrac = 0;
  switch (top) {
    case Top::Tank:
    case Top::Sundress: sleeveFrac = 0; break;
    case Top::TShirt: sleeveFrac = 0.32f; break;
    case Top::LongSleeve: sleeveFrac = 0.93f; break;
    case Top::Sweater: sleeveFrac = 0.93f; sleeveFill = Fill::LightGray; break;
    case Top::Jacket:
    case Top::Coat:
    case Top::LongCoat: sleeveFrac = 0.93f; sleeveFill = Fill::Black; break;
  }
  for (int i = 0; i < 2; ++i) {
    p.limb(sx[i], kShoulderY - 2, hx[i], kHandY - 4, 10, Fill::White);
    if (sleeveFrac > 0) {
      float ex = sx[i] + (hx[i] - sx[i]) * sleeveFrac;
      float ey = (kShoulderY - 2) + (kHandY - 4 - kShoulderY + 2) * sleeveFrac;
      p.limb(sx[i], kShoulderY - 4, ex, ey, 13, sleeveFill);
    }
    p.circle(hx[i], kHandY, 5.5f, gloves ? Fill::Black : Fill::White);
  }
}

void drawHead(Pen& p, Gender g, const Outfit& o) {
  // Neck
  p.poly({{45, 32}, {55, 32}, {55, 46}, {45, 46}}, Fill::White);
  // Hair (black crescent) and face
  p.dot(50, 21, 16, kBlack);
  p.dot(50, g == Gender::Man ? 26.5f : 27.5f, 13.5f, kWhite);
  p.gfx().drawCircle(p.X(50), p.Y(22), p.R(14), kBlack);
  p.gfx().drawCircle(p.X(50), p.Y(22), p.R(14) - 1, kBlack);
  if (o.sunglasses) {
    p.poly({{38, 18}, {48, 18}, {47, 25}, {39, 25}}, Fill::Black);
    p.poly({{52, 18}, {62, 18}, {61, 25}, {53, 25}}, Fill::Black);
    p.line(48, 19, 52, 19, 1, kBlack);
  } else {
    p.dot(44.5f, 22, 1.6f, kBlack);
    p.dot(55.5f, 22, 1.6f, kBlack);
  }
  p.gfx().drawCircleHelper(p.X(50), p.Y(26), p.R(5), 0x4 | 0x8, kBlack);
  if (o.beanie) {
    p.poly({{33, 19}, {35, 10}, {41, 4}, {50, 2}, {59, 4}, {65, 10}, {67, 19}}, Fill::Black);
    p.poly({{32, 14}, {68, 14}, {68, 21}, {32, 21}}, Fill::Black);
    p.dot(50, 1, 4.5f, kBlack);
  }
}

void drawScarf(Pen& p) {
  p.poly({{37, 37}, {63, 37}, {64, 47}, {36, 47}}, Fill::Gray);
  p.poly({{53, 45}, {61, 45}, {63, 76}, {55, 76}}, Fill::Gray);
}

}  // namespace

void drawPerson(Adafruit_GFX& g, int x, int y, float scale, Gender gender,
                const Outfit& o) {
  Pen p(g, x, y, scale);
  if (gender == Gender::Woman) {
    // Long hair behind the head and shoulders.
    p.dot(50, 22, 18, kBlack);
    p.poly({{32, 22}, {68, 22}, {70, 56}, {30, 56}}, Fill::Black);
  }
  drawLegs(p);
  drawBottom(p, o.bottom);
  drawShoes(p, o.shoes);
  drawHead(p, gender, o);
  drawTorso(p, gender, o.top);
  if (o.scarf) drawScarf(p);
  drawArms(p, o.top, o.gloves);
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
