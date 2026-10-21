#pragma once
#include <stddef.h>
#include <stdint.h>

namespace door {

constexpr int kForecastDays = 2;
constexpr int kForecastHours = kForecastDays * 24;
constexpr int kRainThresholdPercent = 10;

struct DayForecast {
  char date[11];        // "YYYY-MM-DD", local to the forecast location
  float feelsHighC;     // apparent_temperature_max
  float feelsLowC;      // apparent_temperature_min
  int8_t precipMaxPct;  // precipitation_probability_max
};

// Plain-old-data so it can live in RTC memory across deep sleep.
struct Forecast {
  bool valid;
  char timezone[40];
  int32_t utcOffsetSeconds;
  DayForecast day[kForecastDays];
  int8_t hourlyPrecipPct[kForecastHours];  // index = day * 24 + hour
  uint8_t hourlyWeatherCode[kForecastHours];
};

struct RainOutlook {
  bool likely;     // max chance >= threshold
  bool snow;       // a likely hour is forecast as snow
  int maxPct;
  int firstHour;   // first/last local hour with chance >= threshold, -1 if none
  int lastHour;
};

void buildForecastUrl(char* buf, size_t len, float lat, float lon);

// Parses an Open-Meteo /v1/forecast response requested with buildForecastUrl.
bool parseOpenMeteo(const char* json, size_t len, Forecast& out);

// Returns the index of the forecast day with the given date, or -1.
int forecastDayIndex(const Forecast& f, const char* isoDate);

// Rain outlook for the rest of the given day starting at fromHour.
RainOutlook rainOutlook(const Forecast& f, int dayIndex, int fromHour,
                        int thresholdPct = kRainThresholdPercent);

// Builds the zippopotam.us lookup URL for a US ZIP code.
void buildZipLookupUrl(char* buf, size_t len, const char* zip);

bool isValidZip(const char* zip);

// Parses a zippopotam.us response. place receives e.g. "San Francisco, CA".
bool parseZipLookup(const char* json, size_t len, float& lat, float& lon,
                    char* place, size_t placeLen);

}  // namespace door
