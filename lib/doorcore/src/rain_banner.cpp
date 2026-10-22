#include "rain_banner.h"

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

void RainBanner::sentence(const RainOutlook& r, bool tomorrow, char* buf, int len) {
  const char* kind = r.snow ? "rain or snow" : "rain";
  char from[12], to[12];
  formatHour(r.firstHour, from, sizeof from);
  formatHour(r.lastHour + 1, to, sizeof to);
  if (r.firstHour < 0) {
    snprintf(buf, len, "%d%% chance of %s %s", r.maxPct, kind, tomorrow ? "tomorrow" : "today");
  } else if (r.firstHour == r.lastHour) {
    snprintf(buf, len, "%d%% chance of %s around %s", r.maxPct, kind, from);
  } else {
    snprintf(buf, len, "Up to %d%% chance of %s, %s to %s", r.maxPct, kind, from, to);
  }
}

void RainBanner::drawHourlyBars(Adafruit_GFX& g, int x0, int x1, int base, int maxH,
                                int labelBaseline, uint16_t c) const {
  const WeatherScreen& s = s_;
  constexpr int kFirst = 6, kLast = 22;
  const float cw = (x1 - x0) / static_cast<float>(kLast - kFirst + 1);
  g.drawFastHLine(x0, base, x1 - x0, c);
  for (int h = kFirst; h <= kLast; ++h) {
    const int cx = x0 + static_cast<int>((h - kFirst) * cw);
    const int w = static_cast<int>(cw) - 6;
    const int p = s.hourlyRain[h];
    if (h >= s.rainFromHour && p > 0) {
      const int bh = std::max(3, p * maxH / 100);
      if (p >= kRainThresholdPercent) {
        g.fillRect(cx + 3, base - bh, w, bh, c);
      } else {
        g.drawRect(cx + 3, base - bh, w, bh, c);
      }
    }
    if ((h - kFirst) % 3 == 0) {
      char label[12];
      formatHour(h, label, sizeof label);
      textCentered(g, kSmall, cx + static_cast<int>(cw / 2), labelBaseline, label, c);
    }
  }
}

int RainBanner::height() const {
  const WeatherScreen& s = s_;
  const bool compact = compact_;
  if (s.rain.likely) return compact ? 98 : 128;
  return compact ? 64 : 118;
}

void RainBanner::draw(Adafruit_GFX& g, int top) const {
  const WeatherScreen& s = s_;
  const bool compact = compact_;
  const int h = height();
  char sub[96];
  if (s.rain.likely) {
    sentence(s.rain, s.tomorrow, sub, sizeof sub);
    g.fillRoundRect(12, top, kScreenW - 24, h, 14, kBlack);
    if (compact) {
      drawUmbrella(g, 64, top + 46, 32, kWhite, kBlack);
      text(g, kTitle, 120, top + 34, "BRING AN UMBRELLA", kWhite);
      const int room = kScreenW - 30 - (120 + textWidth(kTitle, "BRING AN UMBRELLA") + 20);
      if (textWidth(kSmall, sub) > room) snprintf(sub, sizeof sub, "Up to %d%%", s.rain.maxPct);
      textRight(g, kSmall, kScreenW - 30, top + 32, sub, kWhite);
      drawHourlyBars(g, 120, kScreenW - 30, top + 66, 18, top + 87, kWhite);
    } else {
      drawUmbrella(g, 88, top + 58, 44, kWhite, kBlack);
      text(g, kHeadline, 164, top + 46, "BRING AN UMBRELLA", kWhite);
      const GFXfont* subFont = textWidth(kBody, sub) <= kScreenW - 24 - 166 ? kBody : kSmall;
      text(g, subFont, 166, top + 74, sub, kWhite);
      drawHourlyBars(g, 166, kScreenW - 30, top + 100, 20, top + 119, kWhite);
    }
    return;
  }
  g.drawRoundRect(12, top, kScreenW - 24, h, 14, kBlack);
  g.drawRoundRect(13, top + 1, kScreenW - 26, h - 2, 13, kBlack);
  snprintf(sub, sizeof sub, "Chance of rain stays below %d%% (max %d%%)", kRainThresholdPercent,
           s.rain.maxPct);
  if (compact) {
    drawSun(g, 50, top + h / 2, 18, kBlack);
    text(g, kTitle, 92, top + 31, "No umbrella needed", kBlack);
    text(g, kSmall, 93, top + 53, sub, kBlack);
  } else {
    drawSun(g, 88, top + h / 2, 34, kBlack);
    text(g, kTitle, 164, top + 54, "No umbrella needed", kBlack);
    text(g, kBody, 166, top + 92, sub, kBlack);
  }
}

}  // namespace door
