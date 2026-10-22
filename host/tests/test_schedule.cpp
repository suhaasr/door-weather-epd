#include "test.h"

#include <math.h>

#include <map>

#include "schedule.h"
#include "screen.h"

using namespace door;
using namespace test;

TEST(schedule) {
  useTz("EST5EDT,M3.2.0,M11.1.0");
  RefreshWindow w;  // 3..10
  struct tm t;

  // 2:30 AM -> 3:00 AM same day.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 2, 30), w));
  CHECK(t.tm_mday == 21 && t.tm_hour == 3 && t.tm_min == 0);

  // 3:00:20 AM (just refreshed) -> 4:00 AM.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 3, 0, 20), w));
  CHECK(t.tm_mday == 21 && t.tm_hour == 4 && t.tm_min == 0);

  // Woke early at 3:57 because of drift -> next slot is 5:00, not 4:00.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 3, 57), w));
  CHECK(t.tm_hour == 5);

  // 9:00 -> 10:00 (10 AM is the last refresh).
  t = local(nextRefreshTime(localTime(2026, 10, 21, 9, 0, 30), w));
  CHECK(t.tm_mday == 21 && t.tm_hour == 10);

  // 10:00 -> 3:00 next day.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 10, 0, 30), w));
  CHECK(t.tm_mday == 22 && t.tm_hour == 3);

  // Evening setup -> 3:00 next day.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 19, 45), w));
  CHECK(t.tm_mday == 22 && t.tm_hour == 3);

  // Every refresh in a day: exactly 3,4,...,10.
  time_t now = localTime(2026, 10, 21, 0, 0);
  int count = 0;
  for (time_t n = nextRefreshTime(now, w); n < localTime(2026, 10, 22, 0, 0);
       n = nextRefreshTime(n + 20, w)) {
    t = local(n);
    CHECK(t.tm_hour == 3 + count && t.tm_min == 0);
    ++count;
  }
  CHECK(count == 8);

  // DST starts 2026-03-08 at 2 AM: still 3:00 local.
  t = local(nextRefreshTime(localTime(2026, 3, 7, 11, 0), w));
  CHECK(t.tm_mday == 8 && t.tm_hour == 3 && t.tm_min == 0);
  // DST ends 2026-11-01.
  t = local(nextRefreshTime(localTime(2026, 10, 31, 11, 0), w));
  CHECK(t.tm_mday == 1 && t.tm_hour == 3 && t.tm_min == 0);

  // Window wrapping midnight (22..2).
  RefreshWindow wrap{22, 2};
  t = local(nextRefreshTime(localTime(2026, 10, 21, 12, 0), wrap));
  CHECK(t.tm_hour == 22);
  t = local(nextRefreshTime(localTime(2026, 10, 21, 23, 30), wrap));
  CHECK(t.tm_hour == 0 && t.tm_mday == 22);

  struct tm m = local(localTime(2026, 10, 21, 7, 0));
  CHECK(forecastDayOffset(m, 18) == 0);
  m = local(localTime(2026, 10, 21, 18, 0));
  CHECK(forecastDayOffset(m, 18) == 1);

  char buf[16];
  formatClock(local(localTime(2026, 10, 21, 0, 5)), buf, sizeof buf);
  CHECK(strcmp(buf, "12:05 AM") == 0);
  formatClock(local(localTime(2026, 10, 21, 13, 0)), buf, sizeof buf);
  CHECK(strcmp(buf, "1:00 PM") == 0);
  formatIsoDate(local(localTime(2026, 1, 2, 7, 0)), buf, sizeof buf);
  CHECK(strcmp(buf, "2026-01-02") == 0);
  CHECK(strcmp(weekdayName(local(localTime(2026, 10, 21, 7, 0)).tm_wday), "Wednesday") == 0);

  formatHour(0, buf, sizeof buf);
  CHECK(strcmp(buf, "12 AM") == 0);
  formatHour(12, buf, sizeof buf);
  CHECK(strcmp(buf, "12 PM") == 0);
  formatHour(14, buf, sizeof buf);
  CHECK(strcmp(buf, "2 PM") == 0);
  formatHour(24, buf, sizeof buf);
  CHECK(strcmp(buf, "midnight") == 0);
}
