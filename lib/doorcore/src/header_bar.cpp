#include "header_bar.h"

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

void HeaderBar::draw(Adafruit_GFX& g) const {
  const WeatherScreen& s = s_;
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

}  // namespace door
