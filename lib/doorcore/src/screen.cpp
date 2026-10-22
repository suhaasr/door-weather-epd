#include <algorithm>
#include "screen.h"

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "battery.h"
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

void drawRain(Adafruit_GFX& g, const WeatherScreen& s) {
  constexpr int top = 56, h = 118;
  char sub[96];
  if (s.rain.likely) {
    g.fillRoundRect(12, top, kScreenW - 24, h, 14, kBlack);
    drawUmbrella(g, 88, top + 52, 44, kWhite, kBlack);
    text(g, kHeadline, 164, top + 56, "BRING AN UMBRELLA", kWhite);
    rainSentence(s.rain, s.tomorrow, sub, sizeof sub);
    const GFXfont* subFont = textWidth(kBody, sub) <= kScreenW - 24 - 166 ? kBody : kSmall;
    text(g, subFont, 166, top + 96, sub, kWhite);
  } else {
    g.drawRoundRect(12, top, kScreenW - 24, h, 14, kBlack);
    g.drawRoundRect(13, top + 1, kScreenW - 26, h - 2, 13, kBlack);
    drawSun(g, 88, top + h / 2, 34, kBlack);
    text(g, kTitle, 164, top + 54, "No umbrella needed", kBlack);
    snprintf(sub, sizeof sub, "Chance of rain stays below %d%% (max %d%%)",
             kRainThresholdPercent, s.rain.maxPct);
    text(g, kBody, 166, top + 92, sub, kBlack);
  }
}

void drawTemperature(Adafruit_GFX& g, int x, int baseline, float value, char unit) {
  char num[8];
  snprintf(num, sizeof num, "%d", static_cast<int>(lroundf(value)));
  const bool fits = strlen(num) <= 2;
  const GFXfont* f = fits ? kHuge : kHeadline;
  text(g, f, x, baseline, num, kBlack);
  int w = textWidth(f, num);
  int capH = fits ? 63 : 34;
  int r = fits ? 9 : 6;
  drawDegree(g, x + w + r + 2, baseline - capH + r, r, fits ? 4 : 3, kBlack, kWhite);
  char u[2] = {unit, 0};
  text(g, fits ? kTitle : kBodyBold, x + w + 2 * r + 8, baseline - capH + 26, u, kBlack);
}

void drawAttirePanel(Adafruit_GFX& g, int x0, int top, int bottom, const char* label,
                     float temp, char unit, Tier tier) {
  // Content is ~165 px tall; centre it when reminders/battery leave more room.
  const int y = top + std::max(0, (bottom - top - 175) / 2);
  const int tx = x0 + 22;
  const int rx = x0 + 215;
  text(g, kBodyBold, tx, y + 30, label, kBlack);
  drawTemperature(g, tx, y + 112, temp, unit);
  text(g, kTitle, rx, y + 76, tierName(tier), kBlack);
  textWrapped(g, kBody, rx, y + 106, x0 + 400 - rx - 14, 27, tierAdvice(tier), kBlack);
}

void drawLowBatteryBanner(Adafruit_GFX& g, int top, int pct) {
  g.fillRect(0, top, kScreenW, kScreenH - top, kBlack);
  const int h = kScreenH - top;
  drawBatteryIcon(g, 26, top + h / 2 - 16, 60, 32, pct, kWhite);
  char msg[64];
  snprintf(msg, sizeof msg, "BATTERY LOW (%d%%)", pct);
  text(g, kTitle, 112, top + 30, msg, kWhite);
  text(g, kBody, 114, top + 58, "Please recharge with a USB-C cable soon.", kWhite);
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
  drawRain(g, s);

  const bool low = batteryLevel(s.batteryPercent) != BatteryLevel::Ok;
  int top = 184;
  if (s.reminderCount > 0) {
    drawReminderStrip(g, top, 54, s.reminders, s.reminderCount);
    top += 60;
  }
  const int bottom = low ? 408 : 444;
  drawAttirePanel(g, 0, top, bottom, "FEELS LIKE LOW", s.feelsLow, s.unit, s.lowTier);
  g.fillRect(kScreenW / 2 - 1, top + 12, 2, bottom - top - 24, kBlack);
  drawAttirePanel(g, kScreenW / 2, top, bottom, "FEELS LIKE HIGH", s.feelsHigh, s.unit,
                  s.highTier);

  if (low) {
    drawLowBatteryBanner(g, bottom, s.batteryPercent);
  } else {
    g.fillRect(0, bottom, kScreenW, 2, kBlack);
    text(g, kSmall, 16, kScreenH - 12, s.place, kBlack);
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
