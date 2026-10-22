#include "weather_layout.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "battery.h"
#include "extras.h"
#include "icons.h"
#include "text.h"
#include "alert_strip.h"
#include "condition_chips.h"
#include "footer_bar.h"
#include "rain_banner.h"
#include "reminder_strip.h"

namespace door {

WeatherLayout::WeatherLayout(const WeatherScreen& s) {
  const int bottom = FooterBar(s).top();
  const int alertH = AlertStrip(s).height();
  int chipsH = ConditionChips(s).height();
  int remindersH = s.reminderCount > 0 ? ReminderStrip::kHeight : 0;
  auto panelRoom = [&](int rainH) {
    int used = 56 + (alertH ? alertH + 8 : 0) + rainH + 10 + (chipsH ? chipsH + 8 : 0) +
               (remindersH ? remindersH + 6 : 0);
    return bottom - used;
  };
  compact_ = panelRoom(RainBanner(s, false).height()) < 150;
  if (compact_ && remindersH) remindersH = ReminderStrip::kCompactHeight;
  if (compact_ && chipsH && panelRoom(RainBanner(s, true).height()) < 80) chipsH = 0;

  int top = 56;
  alerts_ = {top - 2, alertH};
  if (alertH) top += alertH + 8 - 2;
  const int rainH = RainBanner(s, compact_).height();
  rain_ = {top, rainH};
  top += rainH + 10;
  chips_ = {top, chipsH};
  if (chipsH) top += chipsH + 8;
  reminders_ = {top, remindersH};
  if (remindersH) top += remindersH + 6;
  panels_ = {top, bottom - top};
}

}  // namespace door
