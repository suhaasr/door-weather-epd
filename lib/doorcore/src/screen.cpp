#include <algorithm>
#include "screen.h"

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "battery.h"
#include "extras.h"
#include "icons.h"
#include "fonts/DejaVuSansBold44pt7b.h"

namespace door {

namespace {

const GFXfont* const kSmall = &FreeSans9pt7b;
const GFXfont* const kSmallBold = &FreeSansBold9pt7b;
const GFXfont* const kBody = &FreeSans12pt7b;
const GFXfont* const kBodyBold = &FreeSansBold12pt7b;
const GFXfont* const kTitle = &FreeSansBold18pt7b;
const GFXfont* const kHeadline = &FreeSansBold24pt7b;
const GFXfont* const kHuge = &DejaVuSans_Bold44pt7b;

int textWidth(const GFXfont* f, const char* s) {
  int adv = 0;
  for (const char* p = s; *p; ++p) {
    uint8_t c = static_cast<uint8_t>(*p);
    if (c >= f->first && c <= f->last) adv += f->glyph[c - f->first].xAdvance;
  }
  return adv;
}

void text(Adafruit_GFX& g, const GFXfont* f, int x, int baseline, const char* s,
          uint16_t color) {
  g.setFont(f);
  g.setTextSize(1);
  g.setTextColor(color);
  g.setTextWrap(false);
  g.setCursor(x, baseline);
  g.print(s);
}

void textCentered(Adafruit_GFX& g, const GFXfont* f, int cx, int baseline,
                  const char* s, uint16_t color) {
  text(g, f, cx - textWidth(f, s) / 2, baseline, s, color);
}

void textRight(Adafruit_GFX& g, const GFXfont* f, int right, int baseline,
               const char* s, uint16_t color) {
  text(g, f, right - textWidth(f, s), baseline, s, color);
}

// Greedy word wrap at spaces; '\n' forces a break. Returns lines drawn.
// When centered, x is the centre line.
int textWrapped(Adafruit_GFX& g, const GFXfont* f, int x, int baseline,
                int maxWidth, int lineHeight, const char* s, uint16_t color,
                bool centered = false) {
  char line[128] = "";
  char candidate[128];
  int lines = 0;
  auto flush = [&]() {
    if (!*line) return;
    if (centered) {
      textCentered(g, f, x, baseline + lines * lineHeight, line, color);
    } else {
      text(g, f, x, baseline + lines * lineHeight, line, color);
    }
    ++lines;
    line[0] = 0;
  };
  const char* p = s;
  while (*p) {
    if (*p == ' ') {
      ++p;
      continue;
    }
    if (*p == '\n') {
      flush();
      ++p;
      continue;
    }
    const char* end = p;
    while (*end && *end != ' ' && *end != '\n') ++end;
    snprintf(candidate, sizeof candidate, "%s%s%.*s", line, *line ? " " : "",
             static_cast<int>(end - p), p);
    if (*line && textWidth(f, candidate) > maxWidth) {
      flush();
      snprintf(line, sizeof line, "%.*s", static_cast<int>(end - p), p);
    } else {
      snprintf(line, sizeof line, "%s", candidate);
    }
    p = end;
  }
  flush();
  return lines;
}

void drawHeader(Adafruit_GFX& g, const WeatherScreen& s) {
  char left[48];
  snprintf(left, sizeof left, "%s%s", s.tomorrow ? "Tomorrow, " : "", s.weekday);
  text(g, kBodyBold, 16, 30, left, kBlack);
  text(g, kBody, 16 + textWidth(kBodyBold, left) + 12, 30, s.date, kBlack);

  drawBatteryIcon(g, 748, 12, 38, 20, s.batteryPercent, kBlack);
  char pct[8];
  snprintf(pct, sizeof pct, "%d%%", s.batteryPercent);
  textRight(g, kBody, 740, 30, pct, kBlack);

  char updated[48];
  if (s.stale) {
    snprintf(updated, sizeof updated, "Offline - last update %s", s.updatedAt);
  } else {
    snprintf(updated, sizeof updated, "Updated %s", s.updatedAt);
  }
  int right = 740 - textWidth(kBody, pct) - 20;
  textRight(g, s.stale ? kSmallBold : kSmall, right, 29, updated, kBlack);
  g.fillRect(0, 44, kScreenW, 2, kBlack);
}

void rainSentence(const RainOutlook& r, bool tomorrow, char* buf, int len) {
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

// Bar per hour, 6 AM to 10 PM; solid bars are at or above the rain threshold.
void drawRainBars(Adafruit_GFX& g, const WeatherScreen& s, int x0, int x1, int base, int maxH,
                  int labelBaseline, uint16_t c) {
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

int rainHeight(const WeatherScreen& s, bool compact) {
  if (s.rain.likely) return compact ? 98 : 128;
  return compact ? 64 : 118;
}

void drawRain(Adafruit_GFX& g, const WeatherScreen& s, int top, bool compact) {
  const int h = rainHeight(s, compact);
  char sub[96];
  if (s.rain.likely) {
    rainSentence(s.rain, s.tomorrow, sub, sizeof sub);
    g.fillRoundRect(12, top, kScreenW - 24, h, 14, kBlack);
    if (compact) {
      drawUmbrella(g, 64, top + 46, 32, kWhite, kBlack);
      text(g, kTitle, 120, top + 34, "BRING AN UMBRELLA", kWhite);
      const int room = kScreenW - 30 - (120 + textWidth(kTitle, "BRING AN UMBRELLA") + 20);
      if (textWidth(kSmall, sub) > room) snprintf(sub, sizeof sub, "Up to %d%%", s.rain.maxPct);
      textRight(g, kSmall, kScreenW - 30, top + 32, sub, kWhite);
      drawRainBars(g, s, 120, kScreenW - 30, top + 66, 18, top + 87, kWhite);
    } else {
      drawUmbrella(g, 88, top + 58, 44, kWhite, kBlack);
      text(g, kHeadline, 164, top + 46, "BRING AN UMBRELLA", kWhite);
      const GFXfont* subFont = textWidth(kBody, sub) <= kScreenW - 24 - 166 ? kBody : kSmall;
      text(g, subFont, 166, top + 74, sub, kWhite);
      drawRainBars(g, s, 166, kScreenW - 30, top + 100, 20, top + 119, kWhite);
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

// The most severe alert is large on the first line; the rest follow in small
// text, wrapping onto extra lines as needed.
struct AlertLayout {
  const GFXfont* mainFont;
  int line[kMaxAlerts];  // line (0-based) each alert is drawn on
  int lines;
  int height;
};

constexpr int kAlertX0 = 78, kAlertRight = kScreenW - 30, kAlertSep = 30;
constexpr int kAlertLineH = 24;

int extraAlertWidth(const ScreenAlert& a) {
  return textWidth(kSmallBold, a.event) + 6 + textWidth(kSmall, a.until);
}

void upperCopy(const char* in, char* out, size_t len) {
  size_t i = 0;
  for (; in[i] && i + 1 < len; ++i) out[i] = static_cast<char>(toupper(in[i]));
  out[i] = 0;
}

AlertLayout layoutAlerts(const WeatherScreen& s) {
  AlertLayout l = {kTitle, {}, 0, 0};
  if (s.alertCount <= 0) return l;
  char event[40];
  upperCopy(s.alerts[0].event, event, sizeof event);
  auto mainWidth = [&](const GFXfont* f) {
    return textWidth(f, event) + 14 + textWidth(kBody, s.alerts[0].until);
  };
  if (kAlertX0 + mainWidth(kTitle) > kAlertRight) l.mainFont = kBodyBold;
  int x = kAlertX0 + mainWidth(l.mainFont);
  int line = 0;
  for (int i = 1; i < s.alertCount; ++i) {
    const int w = extraAlertWidth(s.alerts[i]);
    if (x + kAlertSep + w > kAlertRight && x > kAlertX0) {
      ++line;
      x = kAlertX0 - kAlertSep;
    }
    l.line[i] = line;
    x += kAlertSep + w;
  }
  l.lines = line + 1;
  l.height = 44 + line * kAlertLineH + (line ? 4 : 0);
  return l;
}

void drawAlertStrip(Adafruit_GFX& g, const WeatherScreen& s, int top, const AlertLayout& l) {
  g.fillRoundRect(12, top, kScreenW - 24, l.height, 10, kBlack);
  const int cy = top + 22;
  g.fillTriangle(44, cy - 16, 25, cy + 15, 63, cy + 15, kWhite);
  g.fillRect(42, cy - 7, 5, 13, kBlack);
  g.fillRect(42, cy + 9, 5, 4, kBlack);

  char event[40];
  upperCopy(s.alerts[0].event, event, sizeof event);
  text(g, l.mainFont, kAlertX0, cy + (l.mainFont == kTitle ? 12 : 9), event, kWhite);
  int x = kAlertX0 + textWidth(l.mainFont, event) + 14;
  text(g, kBody, x, cy + 9, s.alerts[0].until, kWhite);
  x += textWidth(kBody, s.alerts[0].until);

  int line = 0;
  for (int i = 1; i < s.alertCount; ++i) {
    if (l.line[i] != line) {
      line = l.line[i];
      x = kAlertX0 - kAlertSep;
    }
    const int baseline = line == 0 ? cy + 8 : top + 34 + line * kAlertLineH;
    const ScreenAlert& a = s.alerts[i];
    g.fillCircle(x + kAlertSep / 2, baseline - 5, 3, kWhite);
    x += kAlertSep;
    text(g, kSmallBold, x, baseline, a.event, kWhite);
    x += textWidth(kSmallBold, a.event) + 6;
    text(g, kSmall, x, baseline, a.until, kWhite);
    x += textWidth(kSmall, a.until);
  }
}

int conditionItems(const WeatherScreen& s, char items[3][48]) {
  int n = 0;
  if (s.uvMax >= kSunscreenUvIndex) {
    snprintf(items[n++], 48, "Wear sunscreen (UV %d)", static_cast<int>(lroundf(s.uvMax)));
  }
  if (s.gustMph >= kWindyGustMph) {
    if (s.unit == 'C') {
      snprintf(items[n++], 48, "Windy: gusts %d km/h", static_cast<int>(lroundf(s.gustMph * 1.609f)));
    } else {
      snprintf(items[n++], 48, "Windy: gusts %d mph", static_cast<int>(lroundf(s.gustMph)));
    }
  }
  return n;
}

void drawConditions(Adafruit_GFX& g, char items[3][48], int n, int top, int h) {
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

// Big number (kHuge, or kHeadline when small) with an optional degree + unit.
void drawBigNumber(Adafruit_GFX& g, int x, int baseline, const char* num, char unit, bool small) {
  const GFXfont* f = small ? kHeadline : kHuge;
  text(g, f, x, baseline, num, kBlack);
  if (!unit) return;
  const int w = textWidth(f, num);
  const int capH = small ? 34 : 63;
  const int r = small ? 6 : 9;
  drawDegree(g, x + w + r + 2, baseline - capH + r, r, small ? 3 : 4, kBlack, kWhite);
  char u[2] = {unit, 0};
  text(g, small ? kBodyBold : kTitle, x + w + 2 * r + 8, baseline - capH + 26, u, kBlack);
}

// One of the three equal panels: label, big number, caption underneath.
void drawStatPanel(Adafruit_GFX& g, int x0, int w, int top, int bottom, const char* label,
                   const char* num, char unit, const char* caption) {
  const int h = bottom - top;
  const int tx = x0 + 22;
  const int maxW = w - 30;
  if (h >= 150) {
    const int y = top + (h - 150) / 2;  // centre the ~150 px of content
    text(g, kBodyBold, tx, y + 28, label, kBlack);
    drawBigNumber(g, tx, y + 104, num, unit, false);
    const GFXfont* f = textWidth(kTitle, caption) <= maxW ? kTitle : kBodyBold;
    textWrapped(g, f, tx, y + 140, maxW, 26, caption, kBlack);
  } else if (h >= 112) {
    const int y = top + (h - 112) / 2;
    text(g, kBodyBold, tx, y + 22, label, kBlack);
    drawBigNumber(g, tx, y + 86, num, unit, false);
    text(g, textWidth(kBodyBold, caption) <= maxW ? kBodyBold : kSmallBold, tx, y + 108, caption,
         kBlack);
  } else {
    const int y = top + std::max(0, (h - 80) / 2);
    text(g, kSmallBold, tx, y + 15, label, kBlack);
    drawBigNumber(g, tx, y + 56, num, unit, true);
    text(g, kSmallBold, tx, y + 75, caption, kBlack);
  }
}

void drawLowBatteryBanner(Adafruit_GFX& g, int top, int pct, int daysLeft) {
  g.fillRect(0, top, kScreenW, kScreenH - top, kBlack);
  const int h = kScreenH - top;
  drawBatteryIcon(g, 26, top + h / 2 - 16, 60, 32, pct, kWhite);
  char msg[64];
  snprintf(msg, sizeof msg, "BATTERY LOW (%d%%)", pct);
  text(g, kTitle, 112, top + 30, msg, kWhite);
  char sub[80] = "Please recharge with a USB-C cable soon.";
  if (daysLeft > 0) {
    char left[24];
    formatBatteryLeft(daysLeft, left, sizeof left);
    snprintf(sub, sizeof sub, "Please recharge with a USB-C cable soon (%s left).", left);
  }
  text(g, kBody, 114, top + 58, sub, kWhite);
}

}  // namespace

void formatHour(int hour, char* buf, int len) {
  if (hour == 24) {
    snprintf(buf, len, "midnight");
    return;
  }
  int h = hour % 12;
  if (h == 0) h = 12;
  snprintf(buf, len, "%d %s", h, (hour % 24) < 12 ? "AM" : "PM");
}

void drawReminderStrip(Adafruit_GFX& g, int top, int h, const char* const* items, int n) {
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

void drawWeatherScreen(Adafruit_GFX& g, const WeatherScreen& s) {
  g.fillScreen(kWhite);
  drawHeader(g, s);

  const bool low = batteryLevel(s.batteryPercent) != BatteryLevel::Ok;
  const int bottom = low ? 408 : 444;
  char items[3][48];
  int nItems = conditionItems(s, items);
  const AlertLayout alertLayout = layoutAlerts(s);
  const int alertH = alertLayout.height;
  int chipsH = nItems ? 36 : 0;
  int remindersH = s.reminderCount > 0 ? 54 : 0;
  auto panelRoom = [&](int rainH) {
    int used = 56 + (alertH ? alertH + 8 : 0) + rainH + 10 + (chipsH ? chipsH + 8 : 0) +
               (remindersH ? remindersH + 6 : 0);
    return bottom - used;
  };
  // Shrink the rain banner (and reminders) when everything wouldn't fit.
  const bool compact = panelRoom(rainHeight(s, false)) < 150;
  if (compact && remindersH) remindersH = 48;
  // On extremely busy days the UV/wind/AQI notes give way to the temperatures.
  if (compact && chipsH && panelRoom(rainHeight(s, true)) < 80) {
    chipsH = 0;
    nItems = 0;
  }

  int top = 56;
  if (alertH) {
    drawAlertStrip(g, s, top - 2, alertLayout);
    top += alertH + 8 - 2;
  }
  drawRain(g, s, top, compact);
  top += rainHeight(s, compact) + 10;
  if (chipsH) {
    drawConditions(g, items, nItems, top, chipsH);
    top += chipsH + 8;
  }
  if (remindersH) {
    drawReminderStrip(g, top, remindersH, s.reminders, s.reminderCount);
    top += remindersH + 6;
  }
  constexpr int kPanelW = kScreenW / 3;
  char lo[8], hi[8], aqi[8];
  snprintf(lo, sizeof lo, "%d", static_cast<int>(lroundf(s.feelsLow)));
  snprintf(hi, sizeof hi, "%d", static_cast<int>(lroundf(s.feelsHigh)));
  snprintf(aqi, sizeof aqi, "%s", "--");
  if (s.aqi >= 0) snprintf(aqi, sizeof aqi, "%d", s.aqi);
  drawStatPanel(g, 0, kPanelW, top, bottom, "FEELS LIKE LOW", lo, s.unit, tierName(s.lowTier));
  drawStatPanel(g, kPanelW, kPanelW, top, bottom, "FEELS LIKE HIGH", hi, s.unit,
                tierName(s.highTier));
  drawStatPanel(g, 2 * kPanelW, kScreenW - 2 * kPanelW, top, bottom, "AIR QUALITY", aqi, 0,
                s.aqi >= 0 ? aqiCategory(s.aqi) : "No data");
  for (int k = 1; k < 3; ++k) {
    g.fillRect(k * kPanelW - 1, top + 12, 2, bottom - top - 24, kBlack);
  }

  if (low) {
    drawLowBatteryBanner(g, bottom, s.batteryPercent, s.batteryDaysLeft);
  } else {
    g.fillRect(0, bottom, kScreenW, 2, kBlack);
    text(g, kSmall, 16, kScreenH - 12, s.place, kBlack);
    if (s.batteryDaysLeft > 0) {
      char left[24], msg[48];
      formatBatteryLeft(s.batteryDaysLeft, left, sizeof left);
      snprintf(msg, sizeof msg, "Battery %s left", left);
      textCentered(g, kSmall, kScreenW / 2, kScreenH - 12, msg, kBlack);
    }
    char next[40];
    snprintf(next, sizeof next, "Next update %s", s.nextUpdateAt);
    textRight(g, kSmall, kScreenW - 16, kScreenH - 12, next, kBlack);
  }
}

void drawCriticalBatteryScreen(Adafruit_GFX& g, int pct, const char* lastUpdate) {
  g.fillScreen(kBlack);
  // Large empty battery.
  const int bw = 220, bh = 104, bx = (kScreenW - bw) / 2, by = 34;
  for (int i = 0; i < 8; ++i) g.drawRoundRect(bx + i, by + i, bw - 2 * i, bh - 2 * i, 14, kWhite);
  g.fillRoundRect(bx + bw, by + bh / 2 - 22, 18, 44, 6, kWhite);
  g.fillRect(bx + 16, by + 16, 10, bh - 32, kWhite);

  textCentered(g, kHuge, kScreenW / 2, 240, "RECHARGE ME", kWhite);
  char line[96];
  snprintf(line, sizeof line, "Battery is at %d%%. Plug in a USB-C cable.", pct);
  textCentered(g, kTitle, kScreenW / 2, 300, line, kWhite);
  textCentered(g, kBody, kScreenW / 2, 344,
               "The display has stopped updating the weather.", kWhite);
  if (lastUpdate && *lastUpdate) {
    snprintf(line, sizeof line, "Last weather update: %s", lastUpdate);
    textCentered(g, kBody, kScreenW / 2, 382, line, kWhite);
  }
  textCentered(g, kBody, kScreenW / 2, 440,
               "Updates resume automatically once it is charged.", kWhite);
}

void drawSetupScreen(Adafruit_GFX& g, const char* apName, const char* apPassword,
                     const char* portalIp, int portalMinutes) {
  g.fillScreen(kWhite);
  g.fillRect(0, 0, kScreenW, 70, kBlack);
  text(g, kHeadline, 24, 50, "Wi-Fi setup", kWhite);

  const int x = 92, w = kScreenW - x - 30;
  auto bullet = [&](int n, int y) {
    char num[4];
    snprintf(num, sizeof num, "%d", n);
    g.fillCircle(50, y - 9, 22, kBlack);
    textCentered(g, kTitle, 50, y + 3, num, kWhite);
  };

  int y = 120;
  char line[160];
  bullet(1, y);
  snprintf(line, sizeof line, "On your phone, join the Wi-Fi network \"%s\" with password:", apName);
  y += textWrapped(g, kBody, x, y, w, 30, line, kBlack) * 30;
  const int boxW = textWidth(kTitle, apPassword) + 40;
  g.drawRect(x, y - 16, boxW, 50, kBlack);
  g.drawRect(x + 1, y - 15, boxW - 2, 48, kBlack);
  text(g, kTitle, x + 20, y + 20, apPassword, kBlack);
  y += 80;

  bullet(2, y);
  snprintf(line, sizeof line,
           "A setup page opens automatically. If it doesn't, open http://%s in a browser.",
           portalIp);
  y += textWrapped(g, kBody, x, y, w, 30, line, kBlack) * 30 + 22;

  bullet(3, y);
  textWrapped(g, kBody, x, y, w, 30,
              "Tap \"Configure WiFi\", pick your home network, enter its password and "
              "your 5-digit ZIP code, then tap Save.",
              kBlack);

  g.fillRect(0, kScreenH - 54, kScreenW, 2, kBlack);
  char foot[128];
  snprintf(foot, sizeof foot,
           "Setup closes after %d minutes. To start it again: press RESET, then hold BOOT.",
           portalMinutes);
  textCentered(g, kSmall, kScreenW / 2, kScreenH - 20, foot, kBlack);
}

void drawRemindersScreen(Adafruit_GFX& g, const char* url, const char* altUrl, const char* pin,
                         int idleMinutes) {
  g.fillScreen(kWhite);
  g.fillRect(0, 0, kScreenW, 70, kBlack);
  text(g, kHeadline, 24, 50, "Reminders", kWhite);

  const int x = 92;
  auto bullet = [&](int n, int y) {
    char num[4];
    snprintf(num, sizeof num, "%d", n);
    g.fillCircle(50, y - 9, 22, kBlack);
    textCentered(g, kTitle, 50, y + 3, num, kWhite);
  };
  auto box = [&](int y, const char* s) {
    const int w = textWidth(kTitle, s) + 40;
    g.drawRect(x, y - 16, w, 50, kBlack);
    g.drawRect(x + 1, y - 15, w - 2, 48, kBlack);
    text(g, kTitle, x + 20, y + 20, s, kBlack);
  };

  int y = 120;
  bullet(1, y);
  text(g, kBody, x, y, "On a phone on your home Wi-Fi, open:", kBlack);
  y += 30;
  box(y, url);
  if (altUrl && *altUrl) {
    char alt[64];
    snprintf(alt, sizeof alt, "or %s", altUrl);
    text(g, kBody, x + textWidth(kTitle, url) + 60, y + 18, alt, kBlack);
  }
  y += 90;
  bullet(2, y);
  text(g, kBody, x, y, "Enter this PIN:", kBlack);
  y += 30;
  box(y, pin);
  y += 90;
  bullet(3, y);
  textWrapped(g, kBody, x, y, kScreenW - x - 30, 30,
              "Add or delete reminders, then tap Done. Wi-Fi and ZIP code settings are "
              "on that page too.",
              kBlack);

  g.fillRect(0, kScreenH - 54, kScreenW, 2, kBlack);
  char foot[96];
  snprintf(foot, sizeof foot, "Closes after %d minutes without activity.", idleMinutes);
  textCentered(g, kSmall, kScreenW / 2, kScreenH - 20, foot, kBlack);
}

void drawMessageScreen(Adafruit_GFX& g, const char* title, const char* body, int pct) {
  g.fillScreen(kWhite);
  g.fillRect(0, 0, kScreenW, 70, kBlack);
  text(g, kHeadline, 24, 50, title, kWhite);
  if (pct >= 0) drawBatteryIcon(g, 730, 24, 46, 24, pct, kWhite);
  textWrapped(g, kBody, 30, 130, kScreenW - 60, 34, body, kBlack);
}

}  // namespace door
