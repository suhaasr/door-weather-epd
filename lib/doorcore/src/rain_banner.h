#pragma once
#include "screen.h"

namespace door {

// "BRING AN UMBRELLA" (black, with hourly bars) or "No umbrella needed".
// Compact mode is used on busy days.
class RainBanner {
 public:
  RainBanner(const WeatherScreen& s, bool compact) : s_(s), compact_(compact) {}
  int height() const;
  void draw(Adafruit_GFX& g, int top) const;
  // "Up to 60% chance of rain, 2 PM to 7 PM".
  static void sentence(const RainOutlook& r, bool tomorrow, char* buf, int len);

 private:
  // Bar per hour, 6 AM to 10 PM; solid bars are at or above the rain threshold.
  void drawHourlyBars(Adafruit_GFX& g, int x0, int x1, int base, int maxH, int labelBaseline,
                      uint16_t c) const;
  const WeatherScreen& s_;
  bool compact_;
};

}  // namespace door
