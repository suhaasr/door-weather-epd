#include "test.h"

#include <math.h>

#include <map>

#include "battery.h"

using namespace door;
using namespace test;

TEST(battery_curve) {
  CHECK(batteryPercentFromMillivolts(4250) == 100);
  CHECK(batteryPercentFromMillivolts(4200) == 100);
  CHECK(batteryPercentFromMillivolts(3820) == 50);
  CHECK(batteryPercentFromMillivolts(3600) == 5);
  CHECK(batteryPercentFromMillivolts(3599) == 4);
  CHECK(batteryPercentFromMillivolts(3450) == 1);
  CHECK(batteryPercentFromMillivolts(3300) == 0);
  CHECK(batteryPercentFromMillivolts(3000) == 0);
  // Monotonic over the whole range.
  int prev = 0;
  for (int mv = 3000; mv <= 4300; mv += 5) {
    int p = batteryPercentFromMillivolts(mv);
    CHECK(p >= prev);
    prev = p;
  }
  CHECK(batteryLevel(100) == BatteryLevel::Ok);
  CHECK(batteryLevel(5) == BatteryLevel::Ok);
  CHECK(batteryLevel(4) == BatteryLevel::Low);
  CHECK(batteryLevel(2) == BatteryLevel::Low);
  CHECK(batteryLevel(1) == BatteryLevel::Critical);
  CHECK(batteryLevel(0) == BatteryLevel::Critical);

  CHECK(smoothBatteryPercent(-1, 42) == 42);
  CHECK(smoothBatteryPercent(40, 43) == 40);  // noise up is ignored
  CHECK(smoothBatteryPercent(40, 38) == 38);
  CHECK(smoothBatteryPercent(3, 95) == 95);   // recharged
}

TEST(battery_estimate) {
  CHECK(estimateBatteryDaysLeft(100, 99, 10) == -1);  // not enough drop yet
  CHECK(estimateBatteryDaysLeft(100, 90, 1) == -1);   // not enough time yet
  CHECK(estimateBatteryDaysLeft(100, 90, 14) == 126);
  CHECK(estimateBatteryDaysLeft(100, 4, 96) == 4);
  char buf[32];
  formatBatteryLeft(1, buf, sizeof buf);
  CHECK(strcmp(buf, "~1 day") == 0);
  formatBatteryLeft(9, buf, sizeof buf);
  CHECK(strcmp(buf, "~9 days") == 0);
  formatBatteryLeft(30, buf, sizeof buf);
  CHECK(strcmp(buf, "~4 weeks") == 0);
  formatBatteryLeft(126, buf, sizeof buf);
  CHECK(strcmp(buf, "~4 months") == 0);

}

TEST(battery_tracker) {
  const int64_t day = 86400, t0 = 1800000000;
  BatteryTracker t(0, -1);
  CHECK(t.update(t0, 100));  // first use anchors
  CHECK(t.anchorTime() == t0 && t.anchorPercent() == 100);
  CHECK(t.daysLeft(t0, 100) == -1);
  CHECK(!t.update(t0 + 14 * day, 90));
  CHECK(t.daysLeft(t0 + 14 * day, 90) == 126);
  BatteryTracker half(t0, 50);
  CHECK(!half.update(t0 + day, 53));  // noise: keep the anchor
  CHECK(half.update(t0 + 2 * day, 80) && half.anchorPercent() == 80);  // charged
  CHECK(half.update(t0, 79) && half.anchorTime() == t0);  // clock went backwards
  CHECK(BatteryTracker(t0, 50).daysLeft(t0 - day, 40) == -1);
}
