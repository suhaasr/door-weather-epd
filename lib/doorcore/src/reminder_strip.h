#pragma once
#include "screen.h"

namespace door {

// Bordered strip with the day's reminders, ending in "+N more" if they don't fit.
class ReminderStrip {
 public:
  static constexpr int kHeight = 54, kCompactHeight = 48;
  ReminderStrip(const char* const* items, int count) : items_(items), count_(count) {}
  void draw(Adafruit_GFX& g, int top, int h) const;

 private:
  const char* const* items_;
  int count_;
};

}  // namespace door
