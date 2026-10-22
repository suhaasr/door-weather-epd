// Renders every screen to PNG so layouts can be reviewed without hardware.
//   build/preview <output dir>
#include <stdio.h>
#include <string.h>

#include <string>

#include <Adafruit_GFX.h>

#include "attire.h"
#include "png.h"
#include "screen.h"

using namespace door;

static void save(GFXcanvas1& c, const std::string& dir, const char* name) {
  std::string path = dir + "/" + name + ".png";
  if (!writePng1(path.c_str(), c.getBuffer(), c.width(), c.height())) {
    fprintf(stderr, "failed to write %s\n", path.c_str());
  } else {
    printf("wrote %s\n", path.c_str());
  }
}

static WeatherScreen base() {
  WeatherScreen s = {};
  s.weekday = "Wednesday";
  strcpy(s.date, "2026-10-21");
  strcpy(s.updatedAt, "7:00 AM");
  strcpy(s.nextUpdateAt, "8:00 AM");
  strcpy(s.place, "San Francisco, CA 94110");
  s.unit = 'F';
  s.batteryPercent = 76;
  s.batteryDaysLeft = 63;
  s.uvMax = -1;
  s.gustMph = -1;
  s.aqi = -1;
  return s;
}

// Hourly chances that peak at maxPct between firstHour and lastHour.
static void hourly(WeatherScreen& s, int fromHour) {
  s.rainFromHour = fromHour;
  for (int h = 0; h < 24; ++h) {
    int p = 3;
    if (s.rain.firstHour >= 0 && h >= s.rain.firstHour && h <= s.rain.lastHour) {
      int mid = (s.rain.firstHour + s.rain.lastHour) / 2;
      p = s.rain.maxPct - abs(h - mid) * 8;
      if (p < 15) p = 15;
    } else if (s.rain.firstHour >= 0 && (h == s.rain.firstHour - 1 || h == s.rain.lastHour + 1)) {
      p = 7;
    }
    s.hourlyRain[h] = static_cast<int8_t>(p);
  }
}

static void temps(WeatherScreen& s, float lo, float hi) {
  s.feelsLow = lo;
  s.feelsHigh = hi;
  s.lowTier = tierForFeelsLikeF(lo);
  s.highTier = tierForFeelsLikeF(hi);
}

int main(int argc, char** argv) {
  std::string dir = argc > 1 ? argv[1] : ".";
  GFXcanvas1 c(kScreenW, kScreenH);

  WeatherScreen s = base();
  temps(s, 44, 58);
  s.rain = {true, false, 60, 14, 18};
  hourly(s, 7);
  drawWeatherScreen(c, s);
  save(c, dir, "rainy");

  s = base();
  temps(s, 68, 92);
  s.rain = {false, false, 4, -1, -1};
  strcpy(s.nextUpdateAt, "3:00 AM");
  drawWeatherScreen(c, s);
  save(c, dir, "sunny");

  s = base();
  s.weekday = "Thursday";
  strcpy(s.date, "2026-12-17");
  temps(s, 12, 29);
  s.rain = {true, true, 80, 5, 23};
  hourly(s, 7);
  s.batteryPercent = 3;
  s.batteryDaysLeft = 4;
  drawWeatherScreen(c, s);
  save(c, dir, "low_battery");

  s = base();
  s.weekday = "Thursday";
  strcpy(s.date, "2026-10-22");
  s.tomorrow = true;
  strcpy(s.updatedAt, "7:42 PM");
  strcpy(s.nextUpdateAt, "3:00 AM");
  temps(s, 55, 77);
  s.rain = {true, false, 10, 9, 9};
  hourly(s, 0);
  drawWeatherScreen(c, s);
  save(c, dir, "evening_setup");

  s = base();
  strcpy(s.updatedAt, "6:00 AM");
  s.stale = true;
  temps(s, 62, 73);
  s.rain = {false, false, 0, -1, -1};
  drawWeatherScreen(c, s);
  save(c, dir, "offline");

  drawCriticalBatteryScreen(c, 1, "Wednesday 2026-10-21 7:00 AM");
  save(c, dir, "critical_battery");

  drawSetupScreen(c, "DoorDisplay-Setup", "k7mq4xh2pd", "192.168.4.1", 10);
  save(c, dir, "setup");

  drawMessageScreen(c, "No weather data",
                    "Couldn't connect to Wi-Fi.\nWill try again at 8:00 AM. To change Wi-Fi "
                    "or ZIP code: press RESET, then hold BOOT.",
                    76);
  save(c, dir, "error");

  // Reminders
  s = base();
  s.weekday = "Tuesday";
  strcpy(s.date, "2026-10-20");
  temps(s, 52, 66);
  s.rain = {false, false, 3, -1, -1};
  const char* tue[] = {"Pack lunch", "Take out the recycling"};
  s.reminders[0] = tue[0];
  s.reminders[1] = tue[1];
  s.reminderCount = 2;
  drawWeatherScreen(c, s);
  save(c, dir, "reminders");

  s.rain = {true, false, 70, 6, 13};
  hourly(s, 7);
  temps(s, 38, 47);
  const char* many[] = {"Pack lunch", "Dentist 9:30 AM", "Return library books",
                        "Water the plants"};
  for (int i = 0; i < 4; ++i) s.reminders[i] = many[i];
  s.reminderCount = 4;
  s.batteryPercent = 3;
  s.batteryDaysLeft = 3;
  drawWeatherScreen(c, s);
  save(c, dir, "reminders_low_battery");

  drawRemindersScreen(c, "http://192.168.1.23", "http://doordisplay.local", "482913", 10);
  save(c, dir, "reminders_setup");

  // Weather alert + wind
  s = base();
  s.weekday = "Friday";
  strcpy(s.date, "2026-10-23");
  temps(s, 18, 29);
  s.rain = {true, true, 90, 4, 23};
  hourly(s, 7);
  strcpy(s.alert, "Winter Storm Warning");
  strcpy(s.alertUntil, "until Sat 4 PM");
  s.gustMph = 41;
  drawWeatherScreen(c, s);
  save(c, dir, "alert");

  // Sunny but smoky
  s = base();
  temps(s, 61, 84);
  s.rain = {false, false, 2, -1, -1};
  s.uvMax = 7.2f;
  s.aqi = 162;
  drawWeatherScreen(c, s);
  save(c, dir, "air_quality");

  // Everything at once
  s = base();
  s.weekday = "Tuesday";
  strcpy(s.date, "2026-10-20");
  temps(s, 36, 49);
  s.rain = {true, false, 80, 15, 21};
  hourly(s, 7);
  strcpy(s.alert, "Wind Advisory");
  strcpy(s.alertUntil, "until 11 PM");
  s.alertMore = 1;
  s.gustMph = 48;
  s.aqi = 118;
  s.reminders[0] = "Pack lunch";
  s.reminders[1] = "Take out the recycling";
  s.reminderCount = 2;
  s.batteryPercent = 4;
  s.batteryDaysLeft = 5;
  drawWeatherScreen(c, s);
  save(c, dir, "everything_low_battery");

  return 0;
}
