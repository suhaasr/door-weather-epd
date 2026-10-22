#pragma once
#include "screen.h"

namespace door {

struct Slot {
  int top;
  int height;  // 0 = not shown
};

// Where each part of the weather screen goes. On busy days the rain banner
// and reminders switch to compact sizes, and if even that doesn't leave room
// for the panels the condition chips are dropped.
class WeatherLayout {
 public:
  explicit WeatherLayout(const WeatherScreen& s);
  bool compact() const { return compact_; }
  Slot alerts() const { return alerts_; }
  Slot rain() const { return rain_; }
  Slot chips() const { return chips_; }
  Slot reminders() const { return reminders_; }
  Slot panels() const { return panels_; }

 private:
  bool compact_;
  Slot alerts_, rain_, chips_, reminders_, panels_;
};

}  // namespace door
