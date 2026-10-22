#pragma once
#include "screen.h"

namespace door {

// Weekday, date, update time and battery level along the top.
class HeaderBar {
 public:
  explicit HeaderBar(const WeatherScreen& s) : s_(s) {}
  void draw(Adafruit_GFX& g) const;

 private:
  const WeatherScreen& s_;
};

}  // namespace door
