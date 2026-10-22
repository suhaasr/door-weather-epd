#pragma once
// Optional data shown alongside the forecast: air quality (Open-Meteo) and
// National Weather Service alerts. Either may be missing; the forecast still
// draws without them.
#include <ArduinoJson.h>
#include <stddef.h>

#include "forecast.h"

namespace door {

constexpr float kSunscreenUvIndex = 6;  // "high" on the WHO scale
constexpr int kWindyGustMph = 30;

void buildAirQualityUrl(char* buf, size_t len, float lat, float lon);

// Fills f.hourlyAqi from an Open-Meteo air-quality response. The hourly data
// must start at local midnight of f.day[0].
bool parseAirQuality(const char* json, size_t len, Forecast& f);

// Highest US AQI for the rest of the given day, or -1 if unknown.
int maxAqi(const Forecast& f, int dayIndex, int fromHour);

// "Moderate", "Unhealthy for some", "Unhealthy", ...
const char* aqiCategory(int aqi);

void buildNwsAlertsUrl(char* buf, size_t len, float lat, float lon);

// NWS responses can exceed 100 KB (long alert descriptions); the filter keeps
// only the fields used so they can be parsed straight from the network stream.
void nwsAlertsFilter(JsonDocument& filter);
bool nwsAlertsFromDoc(const JsonDocument& doc, Forecast& f);
bool parseNwsAlerts(const char* json, size_t len, Forecast& f);

// Alerts in effect at some point on isoDate, most severe first. Returns how
// many were written to indices (up to kMaxAlerts).
int alertsFor(const Forecast& f, const char* isoDate, int* indices);

// "until 4 PM" if the alert ends on isoDate, else "until Sat 4 PM".
void formatAlertUntil(const WeatherAlert& a, const char* isoDate, char* buf, size_t len);

// 0 = Sunday. isoDate is "YYYY-MM-DD..."; returns -1 if malformed.
int weekdayOfIsoDate(const char* isoDate);

}  // namespace door
