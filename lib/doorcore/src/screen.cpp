#include "screen.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "alert_strip.h"
#include "battery.h"
#include "condition_chips.h"
#include "extras.h"
#include "footer_bar.h"
#include "header_bar.h"
#include "icons.h"
#include "rain_banner.h"
#include "reminder_strip.h"
#include "stat_panel.h"
#include "text.h"
#include "weather_layout.h"

namespace door {

void drawWeatherScreen(Adafruit_GFX& g, const WeatherScreen& s) {
  g.fillScreen(kWhite);
  HeaderBar(s).draw(g);
  const WeatherLayout layout(s);
  if (layout.alerts().height) AlertStrip(s).draw(g, layout.alerts().top);
  RainBanner(s, layout.compact()).draw(g, layout.rain().top);
  if (layout.chips().height) ConditionChips(s).draw(g, layout.chips().top);
  if (layout.reminders().height) {
    ReminderStrip(s.reminders, s.reminderCount)
        .draw(g, layout.reminders().top, layout.reminders().height);
  }

  const int top = layout.panels().top, bottom = top + layout.panels().height;
  constexpr int kPanelW = kScreenW / 3;
  char lo[8], hi[8], aqi[8];
  snprintf(lo, sizeof lo, "%d", static_cast<int>(lroundf(s.feelsLow)));
  snprintf(hi, sizeof hi, "%d", static_cast<int>(lroundf(s.feelsHigh)));
  snprintf(aqi, sizeof aqi, "%s", "--");
  if (s.aqi >= 0) snprintf(aqi, sizeof aqi, "%d", s.aqi);
  StatPanel("FEELS LIKE LOW", lo, s.unit, tierName(s.lowTier)).draw(g, 0, kPanelW, top, bottom);
  StatPanel("FEELS LIKE HIGH", hi, s.unit, tierName(s.highTier))
      .draw(g, kPanelW, kPanelW, top, bottom);
  StatPanel("AIR QUALITY", aqi, 0, s.aqi >= 0 ? aqiCategory(s.aqi) : "No data",
            s.aqi >= 0 ? StatPanel::aqiWarningLevel(s.aqi) : 0)
      .draw(g, 2 * kPanelW, kScreenW - 2 * kPanelW, top, bottom);
  for (int k = 1; k < 3; ++k) {
    g.fillRect(k * kPanelW - 1, top + 12, 2, bottom - top - 24, kBlack);
  }
  FooterBar(s).draw(g);
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
