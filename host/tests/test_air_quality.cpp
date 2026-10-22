#include "test.h"

#include <math.h>

#include <map>

#include "extras.h"

using namespace door;
using namespace test;

TEST(air_quality) {
  char url[400];
  Forecast f = {};
  std::string json;
  buildAirQualityUrl(url, sizeof url, 37.76f, -122.41f);
  CHECK(strstr(url, "air-quality-api.open-meteo.com") && strstr(url, "us_aqi"));
  json = readFile("fixtures/air_quality_sf.json");
  strcpy(f.day[0].date, "2026-10-20");
  CHECK(!parseAirQuality(json.data(), json.size(), f) && !f.aqiValid);  // other day
  strcpy(f.day[0].date, "2026-10-21");
  CHECK(parseAirQuality(json.data(), json.size(), f) && f.aqiValid);
  CHECK(f.hourlyAqi[0] == 54);
  CHECK(maxAqi(f, 0, 0) == 60);
  CHECK(maxAqi(f, 5, 0) == -1);
  CHECK(strcmp(aqiCategory(40), "Good") == 0);
  CHECK(strcmp(aqiCategory(101), "Unhealthy for some") == 0);
  CHECK(strcmp(aqiCategory(165), "Unhealthy") == 0);
  CHECK(strcmp(aqiCategory(350), "Hazardous") == 0);

}
