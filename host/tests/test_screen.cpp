#include "test.h"

#include <math.h>

#include <map>

#include "screen.h"

using namespace door;
using namespace test;

TEST(screen_weather) {
  GFXcanvas1 canvas(kScreenW, kScreenH);
  WeatherScreen s = {};
  s.weekday = "Wednesday";
  strcpy(s.date, "2026-10-21");
  strcpy(s.updatedAt, "7:00 AM");
  strcpy(s.nextUpdateAt, "8:00 AM");
  strcpy(s.place, "San Francisco, CA 94110");
  s.feelsLow = 41;
  s.feelsHigh = 55;
  s.unit = 'F';
  s.lowTier = Tier::Cold;
  s.highTier = Tier::Cool;
  s.rain = {true, false, 60, 14, 18};
  s.batteryPercent = 80;

  drawWeatherScreen(canvas, s);
  // Rain banner is solid black; bottom strip is mostly white.
  int bannerArea = (kScreenW - 40) * 80;
  CHECK(blackPixels(canvas, 20, 70, kScreenW - 20, 150) > bannerArea / 2);
  CHECK(blackPixels(canvas, 0, 448, kScreenW, kScreenH) < kScreenW * 32 / 4);

  s.rain = {false, false, 4, -1, -1};
  drawWeatherScreen(canvas, s);
  CHECK(blackPixels(canvas, 20, 70, kScreenW - 20, 150) < bannerArea / 4);

  s.batteryPercent = 4;  // low: black banner along the bottom
  drawWeatherScreen(canvas, s);
  CHECK(blackPixels(canvas, 0, 410, kScreenW, kScreenH) > kScreenW * 70 * 3 / 4);
  s.batteryPercent = 5;  // not low yet
  drawWeatherScreen(canvas, s);
  CHECK(blackPixels(canvas, 0, 448, kScreenW, kScreenH) < kScreenW * 32 / 4);

  drawCriticalBatteryScreen(canvas, 1, "Wednesday 2026-10-21 7:00 AM");
  CHECK(blackPixels(canvas, 0, 0, kScreenW, kScreenH) > kScreenW * kScreenH * 3 / 4);
}

TEST(screen_reminders) {
  GFXcanvas1 canvas(kScreenW, kScreenH);
  WeatherScreen s = {};
  s.weekday = "Tuesday";
  strcpy(s.date, "2026-10-20");
  s.feelsLow = 50;
  s.feelsHigh = 60;
  s.unit = 'F';
  s.batteryPercent = 80;
  drawWeatherScreen(canvas, s);
  int without = blackPixels(canvas, 12, 184, kScreenW - 12, 238);
  const char* items[] = {"Pack lunch", "Take out the recycling", "Dentist 9:30 AM",
                         "Water the plants"};
  for (int i = 0; i < 4; ++i) s.reminders[i] = items[i];
  s.reminderCount = 4;
  drawWeatherScreen(canvas, s);
  CHECK(blackPixels(canvas, 12, 184, kScreenW - 12, 238) > without + 2000);
  s.batteryPercent = 3;
  drawWeatherScreen(canvas, s);
  CHECK(blackPixels(canvas, 0, 410, kScreenW, kScreenH) > kScreenW * 70 * 3 / 4);
}

TEST(screen_busy_and_air_quality) {
  // Everything at once still leaves the low-battery banner intact.
  GFXcanvas1 canvas(kScreenW, kScreenH);
  WeatherScreen s = {};
  s.weekday = "Friday";
  strcpy(s.date, "2026-10-23");
  s.unit = 'F';
  s.feelsLow = 20;
  s.feelsHigh = 30;
  s.batteryPercent = 3;
  s.batteryDaysLeft = 4;
  s.rain = {true, true, 90, 6, 22};
  for (int h = 6; h < 23; ++h) s.hourlyRain[h] = 90;
  const char* ev[] = {"Blizzard Warning", "High Wind Warning", "Winter Storm Watch",
                      "Coastal Flood Advisory"};
  for (int i = 0; i < 4; ++i) {
    strcpy(s.alerts[i].event, ev[i]);
    strcpy(s.alerts[i].until, "until Sat 1 AM");
  }
  s.alertCount = 4;
  s.uvMax = 7;
  s.gustMph = 45;
  s.aqi = 160;
  const char* items[] = {"Pack lunch", "Shovel the driveway"};
  s.reminders[0] = items[0];
  s.reminders[1] = items[1];
  s.reminderCount = 2;
  drawWeatherScreen(canvas, s);
  CHECK(blackPixels(canvas, 0, 410, kScreenW, kScreenH) > kScreenW * 70 * 3 / 4);
  CHECK(blackPixels(canvas, 12, 56, kScreenW - 12, 120) > 35000);  // two-line alert strip

  // Air quality gets louder as it worsens: nothing, outlined, solid, inverted.
  WeatherScreen q = {};
  q.weekday = "Wednesday";
  strcpy(q.date, "2026-10-21");
  q.unit = 'F';
  q.batteryPercent = 80;
  int prev = -1;
  for (int aqi : {30, 75, 125, 175}) {
    q.aqi = aqi;
    drawWeatherScreen(canvas, q);
    const int black = blackPixels(canvas, 2 * kScreenW / 3 + 2, 200, kScreenW, 440);
    CHECK(black > prev);
    prev = black;
  }
  CHECK(prev > (kScreenW / 3 - 20) * 200 / 2);  // mostly black when unhealthy
}
