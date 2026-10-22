#pragma once
// HTTPS downloads: forecast (required), air quality and NWS alerts (optional).
#include <Arduino.h>

#include "forecast.h"

namespace door {

class WeatherClient {
 public:
  WeatherClient(float lat, float lon) : lat_(lat), lon_(lon) {}
  bool fetchForecast(Forecast& out);   // retried once
  bool fetchAirQuality(Forecast& f);   // fills f.hourlyAqi
  bool fetchAlerts(Forecast& f);       // fills f.alerts

  // Looks up a 5-digit ZIP's coordinates and "City, ST" name.
  static bool lookupZip(const char* zip, float& lat, float& lon, char* place, size_t placeLen);

 private:
  static bool get(const char* url, String& body);
  float lat_, lon_;
};

}  // namespace door
