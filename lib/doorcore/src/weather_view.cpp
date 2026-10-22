#include "weather_view.h"

#include <stdio.h>
#include <string.h>

#include "attire.h"
#include "extras.h"
#include "schedule.h"

namespace door {

bool WeatherView::build(time_t now, int tomorrowFromHour, WeatherScreen& out) const {
  if (now < kMinValidEpoch) return false;
  struct tm local;
  localtime_r(&now, &local);
  const bool tomorrow = forecastDayOffset(local, tomorrowFromHour) == 1;
  struct tm shown = local;
  if (tomorrow) {
    time_t t = now + 24 * 3600;
    localtime_r(&t, &shown);
  }
  char wanted[11];
  formatIsoDate(shown, wanted, sizeof wanted);
  const int day = forecastDayIndex(f_, wanted);
  if (day < 0) return false;
  fill(day, shown, tomorrow, tomorrow ? 0 : local.tm_hour, out);
  return true;
}

void WeatherView::fill(int day, const struct tm& shownDay, bool tomorrow, int fromHour,
                       WeatherScreen& w) const {
  memset(&w, 0, sizeof w);
  w.weekday = weekdayName(shownDay.tm_wday);
  formatIsoDate(shownDay, w.date, sizeof w.date);
  w.tomorrow = tomorrow;
  struct tm t;
  localtime_r(&in_.fetchedAt, &t);
  formatClock(t, w.updatedAt, sizeof w.updatedAt);
  w.stale = in_.stale;
  localtime_r(&in_.nextUpdate, &t);
  formatClock(t, w.nextUpdateAt, sizeof w.nextUpdateAt);
  snprintf(w.place, sizeof w.place, "%s %s", in_.place, in_.zip);
  const DayForecast& d = f_.day[day];
  const bool fahrenheit = in_.unit == 'F';
  const float lowF = celsiusToFahrenheit(d.feelsLowC), highF = celsiusToFahrenheit(d.feelsHighC);
  w.feelsLow = fahrenheit ? lowF : d.feelsLowC;
  w.feelsHigh = fahrenheit ? highF : d.feelsHighC;
  w.unit = in_.unit;
  w.lowTier = tierForFeelsLikeF(lowF);
  w.highTier = tierForFeelsLikeF(highF);
  w.rain = rainOutlook(f_, day, fromHour);
  for (int h = 0; h < 24; ++h) w.hourlyRain[h] = f_.hourlyPrecipPct[day * 24 + h];
  w.rainFromHour = fromHour;
  w.uvMax = d.uvMax;
  w.gustMph = d.gustMaxMph;
  w.aqi = maxAqi(f_, day, fromHour);
  int idx[kMaxAlerts];
  w.alertCount = alertsFor(f_, w.date, idx);
  for (int i = 0; i < w.alertCount; ++i) {
    const WeatherAlert& a = f_.alerts[idx[i]];
    snprintf(w.alerts[i].event, sizeof w.alerts[i].event, "%s", a.event);
    formatAlertUntil(a, w.date, w.alerts[i].until, sizeof w.alerts[i].until);
  }
  w.batteryPercent = in_.batteryPercent;
  w.batteryDaysLeft = in_.batteryDaysLeft;
}

void WeatherView::addReminders(const ReminderList& list, WeatherScreen& out) {
  int idx[kMaxReminders];
  out.reminderCount = remindersFor(list, out.date, weekdayOfIsoDate(out.date), idx);
  for (int i = 0; i < out.reminderCount; ++i) out.reminders[i] = list.items[idx[i]].text;
}

}  // namespace door
