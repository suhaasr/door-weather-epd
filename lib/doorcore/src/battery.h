#pragma once
#include <stdint.h>

namespace door {

constexpr int kLowBatteryPercent = 5;       // below this: banner along the bottom
constexpr int kCriticalBatteryPercent = 1;  // at or below this: full-screen banner

enum class BatteryLevel { Ok, Low, Critical };

// Maps a resting single-cell LiPo voltage to an approximate state of charge.
// 0% is placed at 3.30 V, where the 3.3 V regulator can no longer hold the rail.
int batteryPercentFromMillivolts(int millivolts);

// Filters ADC noise: the reported charge never goes up unless it jumps by a
// clear margin (i.e. the battery was recharged). previous < 0 means unknown.
int smoothBatteryPercent(int previous, int measured);

BatteryLevel batteryLevel(int percent);

// Days until empty, extrapolated from the drop since the last charge
// (anchorPct, elapsedDays ago). -1 until there's enough history.
int estimateBatteryDaysLeft(int anchorPct, int pct, double elapsedDays);

// Remembers the reading right after the last charge (the "anchor") so the
// drain rate, and from it the days left, can be estimated.
class BatteryTracker {
 public:
  BatteryTracker(int64_t anchorTime, int anchorPercent)
      : anchorTime_(anchorTime), anchorPercent_(anchorPercent) {}
  // Re-anchors on first use, if the clock went backwards, or after a charge
  // (+5% or more). Returns true if the anchor changed and should be saved.
  bool update(int64_t now, int percent);
  int daysLeft(int64_t now, int percent) const;
  int64_t anchorTime() const { return anchorTime_; }
  int anchorPercent() const { return anchorPercent_; }

 private:
  static constexpr int kChargeJumpPercent = 5;
  int64_t anchorTime_;
  int anchorPercent_;
};

// "~5 days", "~3 weeks", "~4 months".
void formatBatteryLeft(int days, char* buf, int len);

}  // namespace door
