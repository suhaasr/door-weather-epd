#include "network.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <stdlib.h>
#include <time.h>

namespace door {

bool Network::connect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < kWifiTimeoutMs) delay(50);
  return connected();
}

bool Network::connected() const { return WiFi.status() == WL_CONNECTED; }

void Network::off() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

bool Network::syncTime(const char* tz) {
  sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
  configTzTime((tz && *tz) ? tz : "UTC0", "pool.ntp.org", "time.google.com");
  const unsigned long start = millis();
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED &&
         millis() - start < kNtpTimeoutMs) {
    delay(50);
  }
  return sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
}

void Network::applyTimeZone(const char* tz) {
  setenv("TZ", (tz && *tz) ? tz : "UTC0", 1);
  tzset();
}

}  // namespace door
