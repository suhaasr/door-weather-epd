#include "condition_chips.h"

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

ConditionChips::ConditionChips(const WeatherScreen& s) {
  int n = 0;
  if (s.uvMax >= kSunscreenUvIndex) {
    snprintf(items_[n++], 48, "Wear sunscreen (UV %d)", static_cast<int>(lroundf(s.uvMax)));
  }
  if (s.gustMph >= kWindyGustMph) {
    if (s.unit == 'C') {
      snprintf(items_[n++], 48, "Windy: gusts %d km/h", static_cast<int>(lroundf(s.gustMph * 1.609f)));
    } else {
      snprintf(items_[n++], 48, "Windy: gusts %d mph", static_cast<int>(lroundf(s.gustMph)));
    }
  }
  count_ = n;
}

void ConditionChips::draw(Adafruit_GFX& g, int top) const {
  const int n = count_;
  const int h = kHeight;
  const auto& items = items_;
  constexpr int kPad = 14, kGap = 10;
  const GFXfont* fonts[] = {kBodyBold, kBody, kSmallBold};
  const GFXfont* f = kSmallBold;
  for (const GFXfont* cand : fonts) {
    int total = 0;
    for (int i = 0; i < n; ++i) total += textWidth(cand, items[i]) + 2 * kPad + (i ? kGap : 0);
    if (total <= kScreenW - 24) {
      f = cand;
      break;
    }
  }
  int x = 12;
  for (int i = 0; i < n; ++i) {
    const int w = textWidth(f, items[i]) + 2 * kPad;
    g.drawRoundRect(x, top, w, h, h / 2, kBlack);
    g.drawRoundRect(x + 1, top + 1, w - 2, h - 2, h / 2 - 1, kBlack);
    text(g, f, x + kPad, top + h / 2 + (f == kSmallBold ? 6 : 8), items[i], kBlack);
    x += w + kGap;
  }
}

}  // namespace door
