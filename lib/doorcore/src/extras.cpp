#include "extras.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "reminders.h"

namespace door {

void buildAirQualityUrl(char* buf, size_t len, float lat, float lon) {
  snprintf(buf, len,
           "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=%.4f&longitude=%.4f"
           "&hourly=us_aqi&timezone=auto&forecast_days=%d",
           lat, lon, kForecastDays);
}

bool parseAirQuality(const char* json, size_t len, Forecast& f) {
  f.aqiValid = false;
  JsonDocument doc;
  if (deserializeJson(doc, json, len)) return false;
  JsonArrayConst hours = doc["hourly"]["time"];
  JsonArrayConst aqi = doc["hourly"]["us_aqi"];
  if (aqi.size() < kForecastHours) return false;
  const char* first = hours[0] | "";
  if (strlen(first) < 16 || strncmp(first, f.day[0].date, 10) != 0 ||
      strcmp(first + 10, "T00:00") != 0) {
    return false;
  }
  for (int h = 0; h < kForecastHours; ++h) {
    f.hourlyAqi[h] = aqi[h].isNull() ? -1 : static_cast<int16_t>(aqi[h].as<int>());
  }
  f.aqiValid = true;
  return true;
}

int maxAqi(const Forecast& f, int dayIndex, int fromHour) {
  if (!f.aqiValid || dayIndex < 0 || dayIndex >= kForecastDays) return -1;
  int best = -1;
  for (int h = fromHour < 0 ? 0 : fromHour; h < 24; ++h) {
    int v = f.hourlyAqi[dayIndex * 24 + h];
    if (v > best) best = v;
  }
  return best;
}

const char* aqiCategory(int aqi) {
  if (aqi <= 50) return "Good";
  if (aqi <= 100) return "Moderate";
  if (aqi <= 150) return "Unhealthy for some";
  if (aqi <= 200) return "Unhealthy";
  if (aqi <= 300) return "Very unhealthy";
  return "Hazardous";
}

void buildNwsAlertsUrl(char* buf, size_t len, float lat, float lon) {
  snprintf(buf, len, "https://api.weather.gov/alerts/active?point=%.4f,%.4f", lat, lon);
}

void nwsAlertsFilter(JsonDocument& filter) {
  filter.clear();
  JsonObject p = filter["features"][0]["properties"].to<JsonObject>();
  for (const char* k : {"event", "severity", "status", "messageType", "onset", "effective",
                        "ends", "expires"}) {
    p[k] = true;
  }
}

namespace {
int severityRank(const char* s) {
  if (!s) return 0;
  if (strcmp(s, "Extreme") == 0) return 4;
  if (strcmp(s, "Severe") == 0) return 3;
  if (strcmp(s, "Moderate") == 0) return 2;
  if (strcmp(s, "Minor") == 0) return 1;
  return 0;
}

void copyTime(char* out, const char* primary, const char* fallback) {
  const char* t = (primary && *primary) ? primary : (fallback ? fallback : "");
  snprintf(out, 17, "%.16s", t);
}
}  // namespace

bool nwsAlertsFromDoc(const JsonDocument& doc, Forecast& f) {
  f.alertsValid = false;
  f.alertCount = 0;
  JsonArrayConst features = doc["features"];
  if (features.isNull()) return false;
  for (JsonObjectConst feature : features) {
    JsonObjectConst p = feature["properties"];
    const char* event = p["event"] | "";
    if (!*event || strcmp(p["status"] | "", "Actual") != 0 ||
        strcmp(p["messageType"] | "", "Cancel") == 0) {
      continue;
    }
    WeatherAlert a = {};
    sanitizeReminderText(event, a.event, sizeof a.event);
    copyTime(a.starts, p["onset"] | "", p["effective"] | "");
    copyTime(a.ends, p["ends"] | "", p["expires"] | "");
    a.severity = static_cast<int8_t>(severityRank(p["severity"] | ""));
    // NWS often sends several messages for one event (e.g. per zone); merge them.
    WeatherAlert* same = nullptr;
    for (int i = 0; i < f.alertCount; ++i) {
      if (strcmp(f.alerts[i].event, a.event) == 0) same = &f.alerts[i];
    }
    if (same) {
      if (*a.starts && (!*same->starts || strcmp(a.starts, same->starts) < 0)) {
        memcpy(same->starts, a.starts, sizeof a.starts);
      }
      if (*a.ends && strcmp(a.ends, same->ends) > 0) memcpy(same->ends, a.ends, sizeof a.ends);
      continue;
    }
    // Keep the most severe few, in order; ties keep NWS order.
    int pos = f.alertCount;
    while (pos > 0 && f.alerts[pos - 1].severity < a.severity) --pos;
    if (pos >= kMaxAlerts) continue;
    int last = f.alertCount < kMaxAlerts ? f.alertCount : kMaxAlerts - 1;
    for (int i = last; i > pos; --i) f.alerts[i] = f.alerts[i - 1];
    f.alerts[pos] = a;
    if (f.alertCount < kMaxAlerts) ++f.alertCount;
  }
  f.alertsValid = true;
  return true;
}

bool parseNwsAlerts(const char* json, size_t len, Forecast& f) {
  JsonDocument filter;
  nwsAlertsFilter(filter);
  JsonDocument doc;
  if (deserializeJson(doc, json, len, DeserializationOption::Filter(filter))) return false;
  return nwsAlertsFromDoc(doc, f);
}

int alertsFor(const Forecast& f, const char* isoDate, int* indices) {
  int n = 0;
  if (!f.alertsValid) return 0;
  for (int i = 0; i < f.alertCount && n < kMaxAlerts; ++i) {
    const WeatherAlert& a = f.alerts[i];
    if (*a.starts && strncmp(a.starts, isoDate, 10) > 0) continue;
    if (*a.ends && strncmp(a.ends, isoDate, 10) < 0) continue;
    indices[n++] = i;  // already sorted by severity
  }
  return n;
}

int weekdayOfIsoDate(const char* iso) {
  if (!iso || strlen(iso) < 10 || !isValidIsoDate(std::string(iso, 10).c_str())) return -1;
  int y = atoi(iso), m = atoi(iso + 5), d = atoi(iso + 8);
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

void formatAlertUntil(const WeatherAlert& a, const char* isoDate, char* buf, size_t len) {
  if (strlen(a.ends) < 16) {
    snprintf(buf, len, "%s", "");
    return;
  }
  const int hour = atoi(a.ends + 11), minute = atoi(a.ends + 14);
  int h12 = hour % 12 == 0 ? 12 : hour % 12;
  char clock[12];
  if (minute) {
    snprintf(clock, sizeof clock, "%d:%02d %s", h12, minute, hour < 12 ? "AM" : "PM");
  } else {
    snprintf(clock, sizeof clock, "%d %s", h12, hour < 12 ? "AM" : "PM");
  }
  static const char* const kDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  int wd = weekdayOfIsoDate(a.ends);
  if (strncmp(a.ends, isoDate, 10) == 0 || wd < 0) {
    snprintf(buf, len, "until %s", clock);
  } else {
    snprintf(buf, len, "until %s %s", kDays[wd], clock);
  }
}

}  // namespace door
