#include "test.h"

#include <math.h>

#include <map>

#include "forecast.h"

using namespace door;
using namespace test;

TEST(forecast) {
  std::string json = readFile("fixtures/open_meteo_sf.json");
  CHECK(!json.empty());
  Forecast f;
  CHECK(parseOpenMeteo(json.data(), json.size(), f));
  CHECK(f.valid);
  CHECK(strcmp(f.timezone, "America/Los_Angeles") == 0);
  CHECK(f.utcOffsetSeconds == -25200);
  CHECK(strcmp(f.day[0].date, "2026-10-21") == 0);
  CHECK(strcmp(f.day[1].date, "2026-10-22") == 0);
  CHECK(f.day[0].feelsHighC > 22.5f && f.day[0].feelsHighC < 22.7f);
  CHECK(f.day[0].feelsLowC > 12.4f && f.day[0].feelsLowC < 12.6f);
  CHECK(f.day[0].precipMaxPct == 31);
  CHECK(f.hourlyPrecipPct[17] == 31);
  CHECK(forecastDayIndex(f, "2026-10-22") == 1);
  CHECK(forecastDayIndex(f, "2026-10-23") == -1);

  // Fixture: 15:00 = 14%, 16 = 26, 17 = 31, 18 = 16, 19 = 10, 20 = 10, 21 = 7.
  RainOutlook r = rainOutlook(f, 0, 7);
  CHECK(r.likely && r.maxPct == 31 && r.firstHour == 15 && r.lastHour == 20 && !r.snow);
  r = rainOutlook(f, 0, 21);
  CHECK(!r.likely && r.maxPct == 7 && r.firstHour == -1);
  r = rainOutlook(f, 0, 7, 50);
  CHECK(!r.likely);

  // Exactly 10% counts as likely.
  Forecast g = f;
  for (int h = 0; h < kForecastHours; ++h) g.hourlyPrecipPct[h] = 0;
  g.hourlyPrecipPct[24 + 8] = 10;
  g.hourlyWeatherCode[24 + 8] = 73;  // snow
  r = rainOutlook(g, 1, 0);
  CHECK(r.likely && r.firstHour == 8 && r.lastHour == 8 && r.snow);
  g.hourlyPrecipPct[24 + 8] = 9;
  CHECK(!rainOutlook(g, 1, 0).likely);

  CHECK(!parseOpenMeteo("{}", 2, f));
  CHECK(!parseOpenMeteo("not json", 8, f));

  char url[320];
  buildForecastUrl(url, sizeof url, 37.75f, -122.4153f);
  CHECK(strstr(url, "latitude=37.7500&longitude=-122.4153") != nullptr);
  CHECK(strstr(url, "apparent_temperature_max") != nullptr);
  CHECK(strstr(url, "forecast_days=2") != nullptr);
}

TEST(forecast_uv_wind) {
  char url[400];
  buildForecastUrl(url, sizeof url, 37.76f, -122.41f);
  CHECK(strstr(url, "uv_index_max,wind_gusts_10m_max") != nullptr);
  CHECK(strstr(url, "wind_speed_unit=mph") != nullptr);
  std::string json = readFile("fixtures/open_meteo_sf.json");
  Forecast f;
  CHECK(parseOpenMeteo(json.data(), json.size(), f));
  CHECK(f.day[0].uvMax < 0 && f.day[0].gustMaxMph < 0);  // fixture predates these fields
  const char* uvJson =
      "{\"daily\":{\"time\":[\"2026-10-21\",\"2026-10-22\"],"
      "\"apparent_temperature_max\":[20,21],\"apparent_temperature_min\":[10,11],"
      "\"precipitation_probability_max\":[0,0],\"uv_index_max\":[7.4,3],"
      "\"wind_gusts_10m_max\":[34.5,12]},\"hourly\":{\"time\":[\"2026-10-21T00:00\"],"
      "\"precipitation_probability\":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
      "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}}";
  CHECK(parseOpenMeteo(uvJson, strlen(uvJson), f));
  CHECK(f.day[0].uvMax > 7.3f && f.day[0].uvMax < 7.5f && f.day[0].gustMaxMph > 34);

}
