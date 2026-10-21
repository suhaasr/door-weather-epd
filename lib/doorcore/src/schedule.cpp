#include "schedule.h"

#include <stdio.h>

namespace door {

bool hourInWindow(int hour, const RefreshWindow& w) {
  if (w.startHour <= w.endHour) return hour >= w.startHour && hour <= w.endHour;
  return hour >= w.startHour || hour <= w.endHour;  // window wraps midnight
}

time_t nextRefreshTime(time_t now, const RefreshWindow& w, int minGapSeconds) {
  time_t earliest = now + minGapSeconds;
  struct tm t;
  localtime_r(&earliest, &t);
  if (t.tm_min != 0 || t.tm_sec != 0) t.tm_hour += 1;
  t.tm_min = 0;
  t.tm_sec = 0;
  for (int i = 0; i < 72; ++i) {
    struct tm probe = t;
    probe.tm_isdst = -1;
    time_t candidate = mktime(&probe);
    if (candidate >= earliest && hourInWindow(probe.tm_hour, w) &&
        probe.tm_min == 0) {
      return candidate;
    }
    t.tm_hour += 1;
  }
  return now + 3600;  // unreachable with a sane window
}

int forecastDayOffset(const struct tm& local, int tomorrowFromHour) {
  return local.tm_hour >= tomorrowFromHour ? 1 : 0;
}

void formatClock(const struct tm& local, char* buf, int len) {
  int h = local.tm_hour % 12;
  if (h == 0) h = 12;
  snprintf(buf, len, "%d:%02d %s", h, local.tm_min,
           local.tm_hour < 12 ? "AM" : "PM");
}

void formatIsoDate(const struct tm& local, char* buf, int len) {
  snprintf(buf, len, "%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1,
           local.tm_mday);
}

const char* weekdayName(int wday) {
  static const char* const kNames[] = {"Sunday",   "Monday", "Tuesday",
                                       "Wednesday", "Thursday", "Friday",
                                       "Saturday"};
  return (wday >= 0 && wday < 7) ? kNames[wday] : "";
}

}  // namespace door
