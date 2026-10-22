#include "test.h"

#include <math.h>

#include "forecast.h"
#include "reminders.h"
#include "weather_view.h"

using namespace door;
using namespace test;

static Forecast sfForecast() {
  std::string json = readFile("fixtures/open_meteo_sf.json");
  Forecast f;
  CHECK(parseOpenMeteo(json.data(), json.size(), f));
  f.aqiValid = true;
  for (int h = 0; h < kForecastHours; ++h) f.hourlyAqi[h] = h < 24 ? 40 : 120;
  f.hourlyAqi[20] = 90;
  f.alertsValid = true;
  f.alertCount = 1;
  strcpy(f.alerts[0].event, "Wind Advisory");
  strcpy(f.alerts[0].starts, "2026-10-21T10:00");
  strcpy(f.alerts[0].ends, "2026-10-21T23:00");
  f.alerts[0].severity = 2;
  return f;
}

TEST(weather_view_today) {
  useTz("PST8PDT,M3.2.0,M11.1.0");
  const Forecast f = sfForecast();
  const time_t now = localTime(2026, 10, 21, 7, 0);
  ViewInputs in = {"San Francisco, CA", "94110", 'F', 76, 63, now, false,
                   localTime(2026, 10, 21, 8, 0)};
  WeatherScreen w;
  CHECK(WeatherView(f, in).build(now, 18, w));
  CHECK(strcmp(w.date, "2026-10-21") == 0 && strcmp(w.weekday, "Wednesday") == 0);
  CHECK(!w.tomorrow && !w.stale);
  CHECK(strcmp(w.place, "San Francisco, CA 94110") == 0);
  CHECK(strcmp(w.updatedAt, "7:00 AM") == 0 && strcmp(w.nextUpdateAt, "8:00 AM") == 0);
  CHECK(w.rain.likely && w.rain.maxPct == 31 && w.rain.firstHour == 15);
  CHECK(w.rainFromHour == 7 && w.hourlyRain[17] == 31);
  CHECK(w.aqi == 90);  // highest for the rest of today
  CHECK(w.alertCount == 1 && strcmp(w.alerts[0].event, "Wind Advisory") == 0);
  CHECK(strcmp(w.alerts[0].until, "until 11 PM") == 0);
  CHECK(w.batteryPercent == 76 && w.batteryDaysLeft == 63);
  CHECK(w.unit == 'F' && fabsf(w.feelsHigh - celsiusToFahrenheit(f.day[0].feelsHighC)) < 0.01f);
  CHECK(w.highTier == tierForFeelsLikeF(w.feelsHigh));
  CHECK(w.uvMax < 0 && w.gustMph < 0);  // fixture has neither

  in.unit = 'C';
  in.stale = true;
  CHECK(WeatherView(f, in).build(now, 18, w));
  CHECK(w.unit == 'C' && fabsf(w.feelsHigh - f.day[0].feelsHighC) < 0.01f && w.stale);
}

TEST(weather_view_evening_and_errors) {
  useTz("PST8PDT,M3.2.0,M11.1.0");
  const Forecast f = sfForecast();
  ViewInputs in = {"San Francisco, CA", "94110", 'F', 76, -1, 0, false, 0};
  WeatherScreen w;
  CHECK(WeatherView(f, in).build(localTime(2026, 10, 21, 19, 0), 18, w));
  CHECK(w.tomorrow && strcmp(w.date, "2026-10-22") == 0 && w.rainFromHour == 0);
  CHECK(w.aqi == 120 && w.alertCount == 0);  // the advisory ends on the 21st
  CHECK(!WeatherView(f, in).build(localTime(2026, 10, 25, 7, 0), 18, w));  // not in forecast
  CHECK(!WeatherView(f, in).build(1000, 18, w));                          // clock not set
}

TEST(weather_view_reminders) {
  useTz("PST8PDT,M3.2.0,M11.1.0");
  const Forecast f = sfForecast();
  ViewInputs in = {"San Francisco, CA", "94110", 'F', 76, -1, 0, false, 0};
  WeatherScreen w;
  CHECK(WeatherView(f, in).build(localTime(2026, 10, 21, 7, 0), 18, w));
  ReminderList list;
  parseReminders("8||Pack lunch\n0|2026-10-21|Dentist\n4||Gym\n0|2026-10-22|Haircut\n", list);
  WeatherView::addReminders(list, w);  // Wednesday 2026-10-21
  CHECK(w.reminderCount == 2);
  CHECK(strcmp(w.reminders[0], "Pack lunch") == 0 && strcmp(w.reminders[1], "Dentist") == 0);
  CHECK(WeatherView(f, in).build(localTime(2026, 10, 21, 19, 0), 18, w));
  WeatherView::addReminders(list, w);  // shows Thursday's
  CHECK(w.reminderCount == 1 && strcmp(w.reminders[0], "Haircut") == 0);
}
