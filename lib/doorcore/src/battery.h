#pragma once

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

}  // namespace door
