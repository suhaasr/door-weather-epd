#include "footer_bar.h"

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

bool FooterBar::lowBattery() const { return batteryLevel(s_.batteryPercent) != BatteryLevel::Ok; }

void FooterBar::draw(Adafruit_GFX& g) const {
  const WeatherScreen& s = s_;
  const int bottom = top();
  if (lowBattery()) {
    drawLowBatteryBanner(g, bottom, s.batteryPercent, s.batteryDaysLeft);
    return;
  }
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

void FooterBar::drawLowBatteryBanner(Adafruit_GFX& g, int top, int pct, int daysLeft) {
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

}  // namespace door
