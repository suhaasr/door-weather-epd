#include "test.h"

#include <math.h>

#include "alert_strip.h"
#include "weather_layout.h"

using namespace door;
using namespace test;

// Every shown slot sits below the previous one, and the panels end at the footer.
static void checkStacked(const WeatherLayout& l, int footerTop) {
  int y = 46;
  for (Slot s : {l.alerts(), l.rain(), l.chips(), l.reminders(), l.panels()}) {
    if (!s.height) continue;
    CHECK(s.top >= y);
    y = s.top + s.height;
  }
  CHECK(l.panels().top + l.panels().height == footerTop);
}

static void addAlert(WeatherScreen& s, const char* event, const char* until) {
  strcpy(s.alerts[s.alertCount].event, event);
  strcpy(s.alerts[s.alertCount].until, until);
  ++s.alertCount;
}

TEST(weather_layout_plain) {
  WeatherScreen s = {};
  s.batteryPercent = 80;
  WeatherLayout l(s);
  CHECK(!l.compact());
  CHECK(l.alerts().height == 0 && l.chips().height == 0 && l.reminders().height == 0);
  CHECK(l.rain().top == 56 && l.rain().height == 118);
  CHECK(l.panels().top == 184);
  checkStacked(l, 444);
  s.batteryPercent = 4;
  checkStacked(WeatherLayout(s), 408);
}

TEST(weather_layout_busy) {
  WeatherScreen s = {};
  s.batteryPercent = 3;
  s.rain = {true, false, 80, 15, 21};
  addAlert(s, "Wind Advisory", "until 11 PM");
  s.gustMph = 48;
  s.reminders[0] = "Pack lunch";
  s.reminderCount = 1;
  WeatherLayout l(s);
  CHECK(l.compact());
  CHECK(l.alerts().height == 44 && l.chips().height > 0);
  CHECK(l.rain().height == 98 && l.reminders().height == 48);
  CHECK(l.panels().height >= 80);
  checkStacked(l, 408);
}

TEST(weather_layout_extreme_drops_chips) {
  WeatherScreen s = {};
  s.batteryPercent = 3;
  s.rain = {true, false, 80, 15, 21};
  addAlert(s, "Excessive Heat Warning", "until Sun 8 PM");
  addAlert(s, "Red Flag Warning", "until 9 PM");
  addAlert(s, "Air Quality Alert", "until Sun 11 PM");
  addAlert(s, "Wind Advisory", "until 6 PM");
  CHECK(AlertStrip(s).lines() == 3);
  s.uvMax = 9;
  s.gustMph = 40;
  s.reminders[0] = "Pack lunch";
  s.reminderCount = 1;
  WeatherLayout l(s);
  CHECK(l.compact() && l.chips().height == 0);
  CHECK(l.panels().height >= 80);
  checkStacked(l, 408);
}
