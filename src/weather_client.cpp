#include "weather_client.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "config.h"
#include "extras.h"

namespace door {

bool WeatherClient::get(const char* url, String& body) {
  WiFiClientSecure client;
  client.setInsecure();  // public, read-only data; skips CA bundle handling
  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(client, url)) return false;
  const bool ok = http.GET() == HTTP_CODE_OK;
  if (ok) body = http.getString();
  http.end();
  return ok;
}

bool WeatherClient::fetchForecast(Forecast& out) {
  char url[320];
  buildForecastUrl(url, sizeof url, lat_, lon_);
  for (int attempt = 0; attempt < 2; ++attempt) {
    String body;
    if (get(url, body) && parseOpenMeteo(body.c_str(), body.length(), out)) return true;
    delay(1000);
  }
  return false;
}

bool WeatherClient::fetchAirQuality(Forecast& f) {
  char url[200];
  buildAirQualityUrl(url, sizeof url, lat_, lon_);
  String body;
  return get(url, body) && parseAirQuality(body.c_str(), body.length(), f);
}

bool WeatherClient::fetchAlerts(Forecast& f) {
  char url[96];
  buildNwsAlertsUrl(url, sizeof url, lat_, lon_);
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  http.useHTTP10(true);  // no chunked encoding, so the body can be parsed as a stream
  if (!http.begin(client, url)) return false;
  http.setUserAgent(kNwsUserAgent);
  http.addHeader("Accept", "application/geo+json");
  bool ok = false;
  if (http.GET() == HTTP_CODE_OK) {
    JsonDocument filter;
    nwsAlertsFilter(filter);
    JsonDocument doc;
    if (!deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter))) {
      ok = nwsAlertsFromDoc(doc, f);
    }
  }
  http.end();
  return ok;
}

bool WeatherClient::lookupZip(const char* zip, float& lat, float& lon, char* place,
                              size_t placeLen) {
  char url[64];
  buildZipLookupUrl(url, sizeof url, zip);
  String body;
  return get(url, body) && parseZipLookup(body.c_str(), body.length(), lat, lon, place, placeLen);
}

}  // namespace door
