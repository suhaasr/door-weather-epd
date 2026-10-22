#include "test.h"

#include <math.h>

#include "alert_strip.h"
#include "condition_chips.h"
#include "footer_bar.h"
#include "rain_banner.h"
#include "screen.h"
#include "stat_panel.h"

using namespace door;
using namespace test;

TEST(rain_banner) {
  char buf[96];
  RainBanner::sentence({true, false, 60, 14, 18}, false, buf, sizeof buf);
  CHECK(strcmp(buf, "Up to 60% chance of rain, 2 PM to 7 PM") == 0);
  RainBanner::sentence({true, true, 30, 9, 9}, false, buf, sizeof buf);
  CHECK(strcmp(buf, "30% chance of rain or snow around 9 AM") == 0);
  RainBanner::sentence({true, false, 40, -1, -1}, true, buf, sizeof buf);
  CHECK(strcmp(buf, "40% chance of rain tomorrow") == 0);

  WeatherScreen s = {};
  s.rain.likely = true;
  CHECK(RainBanner(s, false).height() == 128 && RainBanner(s, true).height() == 98);
  s.rain.likely = false;
  CHECK(RainBanner(s, false).height() == 118 && RainBanner(s, true).height() == 64);

  // Hours before rainFromHour are already past and get no bar.
  GFXcanvas1 c(kScreenW, kScreenH);
  s.rain = {true, false, 90, 6, 22};
  for (int h = 0; h < 24; ++h) s.hourlyRain[h] = 90;
  s.rainFromHour = 6;
  c.fillScreen(kWhite);
  RainBanner(s, false).draw(c, 56);
  const int allBars = blackPixels(c, 166, 136, kScreenW - 30, 156);
  s.rainFromHour = 15;
  c.fillScreen(kWhite);
  RainBanner(s, false).draw(c, 56);
  CHECK(blackPixels(c, 166, 136, kScreenW - 30, 156) > allBars + 1000);  // fewer white bars
}

TEST(condition_chips) {
  WeatherScreen s = {};
  s.unit = 'F';
  s.uvMax = 5.9f;
  s.gustMph = 29.9f;
  CHECK(ConditionChips(s).count() == 0 && ConditionChips(s).height() == 0);
  s.uvMax = 6;
  s.gustMph = 30;
  ConditionChips chips(s);
  CHECK(chips.count() == 2 && chips.height() == ConditionChips::kHeight);
  CHECK(strcmp(chips.item(0), "Wear sunscreen (UV 6)") == 0);
  CHECK(strcmp(chips.item(1), "Windy: gusts 30 mph") == 0);
  s.unit = 'C';
  CHECK(strcmp(ConditionChips(s).item(1), "Windy: gusts 48 km/h") == 0);
  s.aqi = 300;  // air quality has its own panel, not a chip
  CHECK(ConditionChips(s).count() == 2);
}

TEST(stat_panel_aqi_levels) {
  CHECK(StatPanel::aqiWarningLevel(0) == 0);
  CHECK(StatPanel::aqiWarningLevel(50) == 0);
  CHECK(StatPanel::aqiWarningLevel(51) == 1);
  CHECK(StatPanel::aqiWarningLevel(100) == 1);
  CHECK(StatPanel::aqiWarningLevel(101) == 2);
  CHECK(StatPanel::aqiWarningLevel(150) == 2);
  CHECK(StatPanel::aqiWarningLevel(151) == 3);
  CHECK(StatPanel::aqiWarningLevel(500) == 3);
}

TEST(alert_strip) {
  WeatherScreen s = {};
  CHECK(AlertStrip(s).height() == 0);
  strcpy(s.alerts[0].event, "Wind Advisory");
  strcpy(s.alerts[0].until, "until 11 PM");
  s.alertCount = 1;
  CHECK(AlertStrip(s).height() == 44 && AlertStrip(s).lines() == 1);
  strcpy(s.alerts[1].event, "Flood Watch");
  strcpy(s.alerts[1].until, "until Wed 6 AM");
  s.alertCount = 2;
  CHECK(AlertStrip(s).lines() == 1);  // fits beside the main alert
}

TEST(footer_bar) {
  WeatherScreen s = {};
  s.batteryPercent = 5;
  CHECK(!FooterBar(s).lowBattery() && FooterBar(s).top() == 444);
  s.batteryPercent = 4;
  CHECK(FooterBar(s).lowBattery() && FooterBar(s).top() == 408);
}
