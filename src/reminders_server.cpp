#include "reminders_server.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>

#include <string>

#include "config.h"
#include "portal.h"
#include "schedule.h"
#include "screen.h"

namespace door {

RemindersServer::Result RemindersServer::run(const Settings& s) {
  if (!network_.connect()) return Result::NoWifi;
  ReminderList list;
  store_.loadReminders(list);
  ReminderPortal portal(&list, esp_random);
  if (network_.syncTime(s.tz)) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    char today[11];
    formatIsoDate(t, today, sizeof today);
    portal.setToday(today, t.tm_wday);
    if (pruneExpiredReminders(list, today)) store_.saveReminders(list);
  }

  const bool mdns = MDNS.begin(kMdnsName);
  WebServer server(80);
  uint32_t lastActivity = millis();
  server.onNotFound([&]() {
    lastActivity = millis();
    auto arg = [&](const char* name) { return std::string(server.arg(name).c_str()); };
    PortalResponse r =
        portal.handle(server.method() == HTTP_POST ? "POST" : "GET", server.uri().c_str(), arg);
    if (portal.takeChanged()) store_.saveReminders(list);
    server.sendHeader("Cache-Control", "no-store");
    if (!r.location.empty()) server.sendHeader("Location", r.location.c_str());
    server.send(r.status, "text/html; charset=utf-8", r.body.c_str());
  });
  server.begin();

  char url[32], alt[48] = "";
  snprintf(url, sizeof url, "http://%s", WiFi.localIP().toString().c_str());
  if (mdns) snprintf(alt, sizeof alt, "http://%s.local", kMdnsName);
  display_.show([&](Adafruit_GFX& g) {
    drawRemindersScreen(g, url, alt, portal.pin(), kRemindersIdleMinutes);
  });

  const uint32_t idleMs = kRemindersIdleMinutes * 60000UL;
  while (!portal.done() && millis() - lastActivity < idleMs) {
    server.handleClient();
    delay(2);
  }
  for (uint32_t t0 = millis(); millis() - t0 < 300;) {  // let the last response go out
    server.handleClient();
    delay(2);
  }
  server.stop();
  if (mdns) MDNS.end();
  return portal.wifiSetupRequested() ? Result::WifiSetup : Result::Done;
}

}  // namespace door
