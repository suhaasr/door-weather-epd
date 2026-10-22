#pragma once
#include "screen.h"

namespace door {

// Place, battery estimate and next update along the bottom, or the black
// recharge banner when the battery is low.
class FooterBar {
 public:
  explicit FooterBar(const WeatherScreen& s) : s_(s) {}
  bool lowBattery() const;
  int top() const { return lowBattery() ? 408 : 444; }
  void draw(Adafruit_GFX& g) const;

 private:
  static void drawLowBatteryBanner(Adafruit_GFX& g, int top, int pct, int daysLeft);
  const WeatherScreen& s_;
};

}  // namespace door
