#include "reminder_strip.h"

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

void ReminderStrip::draw(Adafruit_GFX& g, int top, int h) const {
  const char* const* items = items_;
  const int n = count_;
  g.fillRoundRect(12, top, kScreenW - 24, h, 12, kBlack);
  g.fillRoundRect(16, top + 4, kScreenW - 32, h - 8, 9, kWhite);
  const int cy = top + h / 2;
  g.fillCircle(44, cy, 16, kBlack);
  g.fillRoundRect(41, cy - 11, 6, 14, 2, kWhite);
  g.fillCircle(44, cy + 8, 3, kWhite);

  const int x0 = 74, right = kScreenW - 30, gap = 34;
  auto total = [&](const GFXfont* f, int count, const char* suffix) {
    int w = 0;
    for (int i = 0; i < count; ++i) w += textWidth(f, items[i]) + (i ? gap : 0);
    if (suffix) w += gap + textWidth(kBody, suffix);
    return w;
  };
  const GFXfont* f = kTitle;
  int shown = n;
  char more[24] = "";
  if (total(kTitle, n, nullptr) > right - x0) {
    f = kBodyBold;
    while (shown > 1) {
      snprintf(more, sizeof more, "+%d more", n - shown);
      if (total(f, shown, shown < n ? more : nullptr) <= right - x0) break;
      --shown;
    }
    if (shown < n) snprintf(more, sizeof more, "+%d more", n - shown);
  }
  const int baseline = cy + (f == kTitle ? 12 : 9);
  int x = x0;
  for (int i = 0; i < shown; ++i) {
    if (i) {
      g.fillCircle(x + gap / 2, cy, 4, kBlack);
      x += gap;
    }
    text(g, f, x, baseline, items[i], kBlack);
    x += textWidth(f, items[i]);
  }
  if (shown < n) text(g, kBody, x + gap, baseline, more, kBlack);
}

}  // namespace door
