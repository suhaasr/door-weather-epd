#pragma once
#include "screen.h"

namespace door {

// Rounded "chips" for conditions worth acting on: sunscreen (UV) and wind.
class ConditionChips {
 public:
  static constexpr int kHeight = 36;
  explicit ConditionChips(const WeatherScreen& s);
  int count() const { return count_; }
  const char* item(int i) const { return items_[i]; }
  int height() const { return count_ ? kHeight : 0; }
  void draw(Adafruit_GFX& g, int top) const;

 private:
  char items_[3][48];
  int count_;
};

}  // namespace door
