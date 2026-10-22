#include "stat_panel.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "battery.h"
#include "extras.h"
#include "icons.h"
#include "text.h"

namespace door {

void StatPanel::drawBigNumber(Adafruit_GFX& g, int x, int baseline, const char* num, char unit,
                              bool small, uint16_t fg, uint16_t bg) {
  const GFXfont* f = small ? kHeadline : kHuge;
  text(g, f, x, baseline, num, fg);
  if (!unit) return;
  const int w = textWidth(f, num);
  const int capH = small ? 34 : 63;
  const int r = small ? 6 : 9;
  drawDegree(g, x + w + r + 2, baseline - capH + r, r, small ? 3 : 4, fg, bg);
  char u[2] = {unit, 0};
  text(g, small ? kBodyBold : kTitle, x + w + 2 * r + 8, baseline - capH + 26, u, fg);
}

void StatPanel::drawWarningIcon(Adafruit_GFX& g, int cx, int cy, int size, bool solid,
                                uint16_t fg, uint16_t bg) {
  const int half = size / 2;
  const int top = cy - half, bottom = cy + half - size / 8;
  if (solid) {
    g.fillTriangle(cx, top, cx - half, bottom, cx + half, bottom, fg);
  } else {
    const int t = std::max(3, size / 12);
    drawThickLine(g, cx, top + t / 2, cx - half + t / 2, bottom - t / 2, t, fg);
    drawThickLine(g, cx, top + t / 2, cx + half - t / 2, bottom - t / 2, t, fg);
    drawThickLine(g, cx - half + t / 2, bottom - t / 2, cx + half - t / 2, bottom - t / 2, t, fg);
  }
  const uint16_t mark = solid ? bg : fg;
  const int bar = std::max(3, size / 9);
  const int markTop = top + size * 3 / 10, markBottom = bottom - size / 4;
  g.fillRect(cx - bar / 2, markTop, bar, markBottom - markTop, mark);
  g.fillRect(cx - bar / 2, bottom - size / 6, bar, bar, mark);
}

int StatPanel::aqiWarningLevel(int aqi) {
  if (aqi <= 50) return 0;
  if (aqi <= 100) return 1;
  if (aqi <= 150) return 2;
  return 3;
}

void StatPanel::draw(Adafruit_GFX& g, int x0, int w, int top, int bottom) const {
  const char* label = label_;
  const char* num = value_;
  const char unit = unit_;
  const char* caption = caption_;
  const int emphasis = warningLevel_;
  const int h = bottom - top;
  const int tx = x0 + 22;
  const int maxW = w - 30;
  const bool inverted = emphasis >= 3;
  const uint16_t fg = inverted ? kWhite : kBlack, bg = inverted ? kBlack : kWhite;
  if (inverted) g.fillRoundRect(x0 + 8, top + 4, w - 16, h - 8, 12, kBlack);
  int numBaseline, capH, iconSize;
  if (h >= 150) {
    const int y = top + (h - 150) / 2;  // centre the ~150 px of content
    text(g, kBodyBold, tx, y + 28, label, fg);
    numBaseline = y + 104;
    drawBigNumber(g, tx, numBaseline, num, unit, false, fg, bg);
    const GFXfont* f = textWidth(kTitle, caption) <= maxW ? kTitle : kBodyBold;
    textWrapped(g, f, tx, y + 140, maxW, 26, caption, fg);
    capH = 63;
    iconSize = 50;
  } else if (h >= 112) {
    const int y = top + (h - 112) / 2;
    text(g, kBodyBold, tx, y + 22, label, fg);
    numBaseline = y + 86;
    drawBigNumber(g, tx, numBaseline, num, unit, false, fg, bg);
    text(g, textWidth(kBodyBold, caption) <= maxW ? kBodyBold : kSmallBold, tx, y + 106, caption,
         fg);
    capH = 63;
    iconSize = 46;
  } else {
    const int y = top + std::max(0, (h - 80) / 2);
    text(g, kSmallBold, tx, y + 15, label, fg);
    numBaseline = y + 56;
    drawBigNumber(g, tx, numBaseline, num, unit, true, fg, bg);
    text(g, kSmallBold, tx, y + 75, caption, fg);
    capH = 34;
    iconSize = 34;
  }
  if (emphasis > 0) {
    // Beside the number if there's room (2 digits), otherwise beside the label.
    const GFXfont* numFont = h >= 112 ? kHuge : kHeadline;
    const int right = x0 + w - 20;
    const bool besideNumber = tx + textWidth(numFont, num) + 14 + iconSize <= right;
    if (besideNumber) {
      drawWarningIcon(g, right - iconSize / 2, numBaseline - capH / 2, iconSize, emphasis >= 2,
                      fg, bg);
    } else {
      const int labelSize = h >= 112 ? 32 : 24;
      const int labelY = h >= 150 ? top + (h - 150) / 2 + 20
                       : h >= 112 ? top + (h - 112) / 2 + 14
                                  : top + std::max(0, (h - 80) / 2) + 9;
      drawWarningIcon(g, right - labelSize / 2, labelY, labelSize, emphasis >= 2, fg, bg);
    }
  }
}

}  // namespace door
