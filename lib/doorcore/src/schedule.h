#pragma once
#include <time.h>

namespace door {

// Refreshes happen at the top of every hour from startHour to endHour
// (inclusive, local time). The default is 3:00, 4:00, ... 10:00 AM.
struct RefreshWindow {
  int startHour = 3;
  int endHour = 10;
};

bool hourInWindow(int hour, const RefreshWindow& window);

// Next top-of-hour refresh that is at least minGapSeconds after now.
// Uses the process time zone (TZ), so DST transitions are handled.
time_t nextRefreshTime(time_t now, const RefreshWindow& window,
                       int minGapSeconds = 300);

// 0 = show today's forecast, 1 = show tomorrow's. Late in the day (e.g. when
// the device is first set up in the evening) tomorrow is more useful.
int forecastDayOffset(const struct tm& local, int tomorrowFromHour);

// Formats "7:00 AM".
void formatClock(const struct tm& local, char* buf, int len);

// Formats "2026-10-21".
void formatIsoDate(const struct tm& local, char* buf, int len);

const char* weekdayName(int tmWday);

}  // namespace door
