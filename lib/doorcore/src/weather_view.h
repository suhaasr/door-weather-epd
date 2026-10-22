#pragma once
// Turns a downloaded forecast into the WeatherScreen that gets drawn.
#include <time.h>

#include "forecast.h"
#include "reminders.h"
#include "screen.h"

namespace door {

constexpr time_t kMinValidEpoch = 1700000000;  // Nov 2023: anything earlier means no clock

struct ViewInputs {
  const char* place;  // "San Francisco, CA"
  const char* zip;
  char unit;          // 'F' or 'C'
  int batteryPercent;
  int batteryDaysLeft;
  time_t fetchedAt;
  bool stale;         // the latest download failed
  time_t nextUpdate;
};

class WeatherView {
 public:
  WeatherView(const Forecast& f, const ViewInputs& in) : f_(f), in_(in) {}

  // Fills `out` for today, or for tomorrow from tomorrowFromHour on (local
  // time). Returns false if the clock isn't set or the forecast doesn't cover
  // that day.
  bool build(time_t now, int tomorrowFromHour, WeatherScreen& out) const;

  // Adds the reminders for out.date. `out` points into `list`, so `list` must
  // outlive it.
  static void addReminders(const ReminderList& list, WeatherScreen& out);

 private:
  void fill(int day, const struct tm& shownDay, bool tomorrow, int fromHour,
            WeatherScreen& w) const;
  const Forecast& f_;
  const ViewInputs& in_;
};

}  // namespace door
