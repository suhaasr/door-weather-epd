// Door weather display firmware (ESP32 + 7.5" e-paper).
//
// Every boot is short-lived: read the battery, fetch the forecast, draw, and
// go back to deep sleep until the next refresh slot. Nothing runs between
// refreshes, which is what makes months of battery life possible.

#include <Arduino.h>
#include <string.h>
#include <time.h>

#include "battery.h"
#include "config.h"
#include "display.h"
#include "drift.h"
#include "network.h"
#include "power.h"
#include "reminders_server.h"
#include "schedule.h"
#include "screen.h"
#include "setup_portal.h"
#include "storage.h"
#include "tz.h"
#include "weather_client.h"
#include "weather_view.h"

using namespace door;

namespace {

EpaperDisplay display;
Network network;
SettingsStore store;

void showCriticalBattery(RtcState& rtc, int battery) {
  char last[48] = "";
  if (rtc.forecastFetchedAt > kMinValidEpoch) {
    struct tm t;
    localtime_r(&rtc.forecastFetchedAt, &t);
    char date[11], clock[12];
    formatIsoDate(t, date, sizeof date);
    formatClock(t, clock, sizeof clock);
    snprintf(last, sizeof last, "%s %s %s", weekdayName(t.tm_wday), date, clock);
  }
  display.show([&](Adafruit_GFX& g) { drawCriticalBatteryScreen(g, battery, last); });
}

// Downloads the forecast plus optional extras into RTC memory and adopts the
// location's time zone. Returns false if the forecast itself failed.
bool refreshForecast(RtcState& rtc, Settings& settings, time_t now) {
  WeatherClient client(settings.lat, settings.lon);
  Forecast f;
  if (!client.fetchForecast(f)) return false;
  client.fetchAirQuality(f);
  if (!client.fetchAlerts(f) && rtc.forecast.alertsValid) {
    // Keep the last known alerts; they're filtered by date when drawn.
    f.alertsValid = true;
    f.alertCount = rtc.forecast.alertCount;
    memcpy(f.alerts, rtc.forecast.alerts, sizeof f.alerts);
  }
  rtc.forecast = f;
  rtc.forecastFetchedAt = now;
  char tz[48];
  posixTzForIana(f.timezone, f.utcOffsetSeconds, tz, sizeof tz);
  if (strcmp(tz, settings.tz) != 0) {
    snprintf(settings.tz, sizeof settings.tz, "%s", tz);
    store.saveTz(tz);
    Network::applyTimeZone(tz);
  }
  return true;
}

// Days of battery left. The tracker's anchor lives in flash (RTC memory is
// lost on RESET) and is only rewritten after a charge.
int batteryDaysLeft(int pct, time_t now) {
  if (now < kMinValidEpoch) return -1;
  BatteryTracker tracker = store.loadBatteryTracker();
  if (tracker.update(now, pct)) store.saveBatteryTracker(tracker);
  return tracker.daysLeft(now, pct);
}

void showNoData(bool online, time_t next, int battery) {
  char body[256], when[12];
  struct tm nt;
  localtime_r(&next, &nt);
  formatClock(nt, when, sizeof when);
  snprintf(body, sizeof body,
           "%s\nWill try again at %s. To change Wi-Fi or ZIP code: press RESET, then "
           "hold BOOT.",
           !online ? "Couldn't connect to Wi-Fi." : "Couldn't download today's forecast.", when);
  display.show([&](Adafruit_GFX& g) { drawMessageScreen(g, "No weather data", body, battery); });
}

}  // namespace

void setup() {
  const bool timerWake = Power::wokeFromTimer();
  RtcState& rtc = Power::rtc();

  // Measure before Wi-Fi loads the battery.
  int battery = Power::readBatteryPercent();
  battery = timerWake ? smoothBatteryPercent(rtc.batteryPercent, battery) : battery;
  rtc.batteryPercent = battery;

  if (batteryLevel(battery) == BatteryLevel::Critical) {
    if (!rtc.criticalShown) {
      showCriticalBattery(rtc, battery);
      rtc.criticalShown = true;
    }
    rtc.sleepStartedAt = 0;
    Power::sleepFor(kCriticalRecheckMinutes * 60);
  }
  const bool recoveredFromCritical = rtc.criticalShown;
  rtc.criticalShown = false;

  Settings settings = store.load();
  Network::applyTimeZone(settings.tz);
  SetupPortal setupPortal(store, display, network);

  // RESET then BOOT opens the reminders page (which links to Wi-Fi setup); if
  // the home network is unreachable, go straight to the Wi-Fi setup hotspot.
  bool wantSetup = !settings.hasLocation;
  if (!timerWake && !wantSetup && SetupPortal::bootButtonHeld()) {
    wantSetup = RemindersServer(store, display, network).run(settings) !=
                RemindersServer::Result::Done;
  }
  if (wantSetup && !setupPortal.run(settings, battery)) Power::sleepUntilReset();

  bool online = network.connected() || network.connect();
  if (!online && !timerWake) {
    // Manual reset and the network is unreachable: offer setup right away.
    if (!setupPortal.run(settings, battery)) Power::sleepUntilReset();
    online = true;
  }

  const bool synced = online && network.syncTime(settings.tz);
  const time_t now = time(nullptr);
  if (synced && timerWake && rtc.sleepStartedAt > 0) {
    double actual = difftime(now, rtc.sleepStartedAt) - millis() / 1000.0;
    rtc.sleepCorrection =
        updateSleepCorrection(rtc.sleepCorrection, rtc.programmedSleepS, actual);
  }

  const bool fetched = online && refreshForecast(rtc, settings, now);
  network.off();

  RefreshWindow window;
  window.startHour = kRefreshStartHour;
  window.endHour = kRefreshEndHour;
  const time_t next = nextRefreshTime(now, window);

  ViewInputs in = {settings.place, settings.zip, kTemperatureUnit, battery,
                   batteryDaysLeft(battery, now), rtc.forecastFetchedAt, !fetched, next};
  WeatherScreen w;
  if (WeatherView(rtc.forecast, in).build(now, kShowTomorrowFromHour, w)) {
    ReminderList reminders;
    store.loadReminders(reminders);
    struct tm local;
    localtime_r(&now, &local);
    char today[11];
    formatIsoDate(local, today, sizeof today);
    if (pruneExpiredReminders(reminders, today)) store.saveReminders(reminders);
    WeatherView::addReminders(reminders, w);
    display.show([&](Adafruit_GFX& g) { drawWeatherScreen(g, w); });
  } else if (!timerWake || recoveredFromCritical || fetched || rtc.forecast.valid) {
    showNoData(online, next, battery);
    rtc.forecast.valid = false;  // don't redraw the same error every hour
  }

  rtc.sleepStartedAt = synced ? time(nullptr) : 0;
  Power::sleepFor(difftime(next, time(nullptr)));
}

void loop() {}
