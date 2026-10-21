#include "forecast.h"

#include <ArduinoJson.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace door {

void buildForecastUrl(char* buf, size_t len, float lat, float lon) {
  snprintf(buf, len,
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&daily=apparent_temperature_max,apparent_temperature_min,"
           "precipitation_probability_max"
           "&hourly=precipitation_probability,weather_code"
           "&timezone=auto&forecast_days=%d",
           lat, lon, kForecastDays);
}

bool parseOpenMeteo(const char* json, size_t len, Forecast& out) {
  memset(&out, 0, sizeof(out));
  JsonDocument doc;
  if (deserializeJson(doc, json, len)) return false;

  JsonArrayConst dates = doc["daily"]["time"];
  JsonArrayConst highs = doc["daily"]["apparent_temperature_max"];
  JsonArrayConst lows = doc["daily"]["apparent_temperature_min"];
  JsonArrayConst pmax = doc["daily"]["precipitation_probability_max"];
  JsonArrayConst hours = doc["hourly"]["time"];
  JsonArrayConst hprecip = doc["hourly"]["precipitation_probability"];
  JsonArrayConst hcode = doc["hourly"]["weather_code"];
  if (dates.size() < kForecastDays || highs.size() < kForecastDays ||
      lows.size() < kForecastDays || hprecip.size() < kForecastHours) {
    return false;
  }
  // Hourly data must start at local midnight of the first day.
  const char* firstHour = hours[0] | "";
  if (strlen(firstHour) < 16 || strcmp(firstHour + 10, "T00:00") != 0) return false;

  for (int d = 0; d < kForecastDays; ++d) {
    DayForecast& day = out.day[d];
    snprintf(day.date, sizeof(day.date), "%s", dates[d] | "");
    if (highs[d].isNull() || lows[d].isNull()) return false;
    day.feelsHighC = highs[d].as<float>();
    day.feelsLowC = lows[d].as<float>();
    day.precipMaxPct = static_cast<int8_t>(pmax[d] | 0);
  }
  for (int h = 0; h < kForecastHours; ++h) {
    out.hourlyPrecipPct[h] = static_cast<int8_t>(hprecip[h] | 0);
    out.hourlyWeatherCode[h] = static_cast<uint8_t>(hcode[h] | 0);
  }
  snprintf(out.timezone, sizeof(out.timezone), "%s", doc["timezone"] | "");
  out.utcOffsetSeconds = doc["utc_offset_seconds"] | 0;
  out.valid = true;
  return true;
}

int forecastDayIndex(const Forecast& f, const char* isoDate) {
  if (!f.valid) return -1;
  for (int d = 0; d < kForecastDays; ++d) {
    if (strcmp(f.day[d].date, isoDate) == 0) return d;
  }
  return -1;
}

namespace {
bool isSnowCode(int code) {
  return (code >= 71 && code <= 77) || code == 85 || code == 86;
}
}  // namespace

RainOutlook rainOutlook(const Forecast& f, int dayIndex, int fromHour,
                        int thresholdPct) {
  RainOutlook r{false, false, 0, -1, -1};
  if (dayIndex < 0 || dayIndex >= kForecastDays) return r;
  if (fromHour < 0) fromHour = 0;
  for (int h = fromHour; h < 24; ++h) {
    int i = dayIndex * 24 + h;
    int p = f.hourlyPrecipPct[i];
    if (p > r.maxPct) r.maxPct = p;
    if (p >= thresholdPct) {
      if (r.firstHour < 0) r.firstHour = h;
      r.lastHour = h;
      if (isSnowCode(f.hourlyWeatherCode[i])) r.snow = true;
    }
  }
  r.likely = r.maxPct >= thresholdPct;
  return r;
}

bool isValidZip(const char* zip) {
  if (!zip || strlen(zip) != 5) return false;
  for (int i = 0; i < 5; ++i) {
    if (!isdigit(static_cast<unsigned char>(zip[i]))) return false;
  }
  return true;
}

void buildZipLookupUrl(char* buf, size_t len, const char* zip) {
  snprintf(buf, len, "https://api.zippopotam.us/us/%s", zip);
}

bool parseZipLookup(const char* json, size_t len, float& lat, float& lon,
                    char* place, size_t placeLen) {
  JsonDocument doc;
  if (deserializeJson(doc, json, len)) return false;
  JsonObjectConst p = doc["places"][0];
  if (p.isNull()) return false;
  // zippopotam returns coordinates as strings.
  const char* latStr = p["latitude"] | "";
  const char* lonStr = p["longitude"] | "";
  if (!*latStr || !*lonStr) return false;
  lat = static_cast<float>(atof(latStr));
  lon = static_cast<float>(atof(lonStr));
  snprintf(place, placeLen, "%s, %s", p["place name"] | "",
           p["state abbreviation"] | "");
  return true;
}

}  // namespace door
