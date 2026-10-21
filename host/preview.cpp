// Renders every screen to PNG so layouts can be reviewed without hardware.
//   build/preview <output dir>
#include <stdio.h>
#include <string.h>

#include <string>

#include <Adafruit_GFX.h>

#include "attire.h"
#include "figures.h"
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
  return s;
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
  s.batteryPercent = 3;
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

  drawSetupScreen(c, "DoorDisplay-Setup", "192.168.4.1", 10);
  save(c, dir, "setup");

  drawMessageScreen(c, "No weather data",
                    "Couldn't connect to Wi-Fi.\nWill try again at 8:00 AM. To change Wi-Fi "
                    "or ZIP code: press RESET, then hold BOOT.",
                    76);
  save(c, dir, "error");

  // Every outfit tier for both figures.
  c.fillScreen(kWhite);
  const Tier tiers[] = {Tier::Freezing, Tier::Cold, Tier::Chilly, Tier::Cool,
                        Tier::Mild,     Tier::Warm, Tier::Hot};
  for (int i = 0; i < 7; ++i) {
    int x = 8 + i * 113;
    drawPerson(c, x, 20, 0.52f, Gender::Man, outfitFor(tiers[i], Gender::Man));
    drawPerson(c, x + 54, 20, 0.52f, Gender::Woman, outfitFor(tiers[i], Gender::Woman));
    drawPerson(c, x - 2, 150, 1.1f, i % 2 ? Gender::Woman : Gender::Man,
               outfitFor(tiers[i], i % 2 ? Gender::Woman : Gender::Man));
    c.setFont(nullptr);
    c.setTextColor(kBlack);
    c.setCursor(x + 20, 135);
    c.print(tierName(tiers[i]));
  }
  save(c, dir, "outfits");
  return 0;
}
