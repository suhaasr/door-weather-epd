#include "battery.h"

namespace door {

namespace {
struct Point {
  int mv;
  int pct;
};
// Typical resting discharge curve of a 3.7 V LiPo cell at low load.
constexpr Point kCurve[] = {
    {4200, 100}, {4100, 90}, {4000, 80}, {3920, 70}, {3870, 60}, {3820, 50},
    {3790, 40},  {3770, 30}, {3740, 20}, {3700, 12}, {3650, 7},  {3600, 5},
    {3500, 2},   {3450, 1},  {3300, 0},
};
constexpr int kCurveLen = sizeof(kCurve) / sizeof(kCurve[0]);
constexpr int kRechargeJumpPercent = 10;
}  // namespace

int batteryPercentFromMillivolts(int mv) {
  if (mv >= kCurve[0].mv) return 100;
  if (mv <= kCurve[kCurveLen - 1].mv) return 0;
  for (int i = 1; i < kCurveLen; ++i) {
    const Point& hi = kCurve[i - 1];
    const Point& lo = kCurve[i];
    if (mv >= lo.mv) {
      // Round down so that the "1%" warning shows up early rather than late.
      return lo.pct + (mv - lo.mv) * (hi.pct - lo.pct) / (hi.mv - lo.mv);
    }
  }
  return 0;
}

int smoothBatteryPercent(int previous, int measured) {
  if (previous < 0 || previous > 100) return measured;
  if (measured >= previous + kRechargeJumpPercent) return measured;
  return measured < previous ? measured : previous;
}

BatteryLevel batteryLevel(int percent) {
  if (percent <= kCriticalBatteryPercent) return BatteryLevel::Critical;
  if (percent < kLowBatteryPercent) return BatteryLevel::Low;
  return BatteryLevel::Ok;
}

}  // namespace door
