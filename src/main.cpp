// Door weather display firmware (ESP32 + 7.5" e-paper).
//
// Every boot is short-lived: read the battery, fetch the forecast, draw, and
// go back to deep sleep until the next refresh slot. Nothing runs between
// refreshes, which is what makes months of battery life possible.

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <esp_sleep.h>
#include <esp_sntp.h>
#include <time.h>

#include "attire.h"
#include "battery.h"
#include "config.h"
#include "drift.h"
#include "forecast.h"
#include "schedule.h"
#include "screen.h"
#include "setup_password.h"
#include "tz.h"

using namespace door;

namespace {

GxEPD2_BW<DOOR_EPD_DRIVER, DOOR_EPD_DRIVER::HEIGHT> display(
    DOOR_EPD_DRIVER(kPinEpdCs, kPinEpdDc, kPinEpdRst, kPinEpdBusy));

constexpr uint32_t kRtcMagic = 0xD00D0003;
constexpr uint32_t kWifiTimeoutMs = 15000;
constexpr uint32_t kNtpTimeoutMs = 10000;
constexpr time_t kMinValidEpoch = 1700000000;  // Nov 2023

// Survives deep sleep (lost on power loss or RESET).
struct RtcState {
  uint32_t magic;
  int batteryPercent;
  float sleepCorrection;
  time_t sleepStartedAt;    // NTP-accurate time we went to sleep, 0 if unknown
  double programmedSleepS;  // value handed to the sleep timer
  bool criticalShown;
  time_t forecastFetchedAt;
  Forecast forecast;
};
RTC_DATA_ATTR RtcState rtc;

struct Settings {
  char zip[6];
  float lat;
  float lon;
  char place[40];
  char tz[48];
  bool hasLocation;
};

Settings loadSettings() {
  Settings s{};
  Preferences p;
  p.begin("door", true);
  p.getString("zip", s.zip, sizeof s.zip);
  p.getString("place", s.place, sizeof s.place);
  p.getString("tz", s.tz, sizeof s.tz);
  s.lat = p.getFloat("lat", NAN);
  s.lon = p.getFloat("lon", NAN);
  p.end();
  s.hasLocation = isValidZip(s.zip) && !isnan(s.lat) && !isnan(s.lon);
  return s;
}

void saveLocation(const Settings& s) {
  Preferences p;
  p.begin("door", false);
  p.putString("zip", s.zip);
  p.putString("place", s.place);
  p.putFloat("lat", s.lat);
  p.putFloat("lon", s.lon);
  p.end();
}

void saveTz(const char* tz) {
  Preferences p;
  p.begin("door", false);
  p.putString("tz", tz);
  p.end();
}

void applyTz(const char* tz) {
  setenv("TZ", (tz && *tz) ? tz : "UTC0", 1);
  tzset();
}

int readBatteryPercent() {
  pinMode(kPinBatteryAdc, INPUT);
  uint32_t sum = 0;
  constexpr int kSamples = 16;
  for (int i = 0; i < kSamples; ++i) sum += analogReadMilliVolts(kPinBatteryAdc);
  int mv = static_cast<int>(sum / kSamples * kBatteryDividerRatio) + kBatteryMillivoltOffset;
  return batteryPercentFromMillivolts(mv);
}

// ---- Display -------------------------------------------------------------

void displayBegin() {
  if (kPinEpdPower >= 0) {
    pinMode(kPinEpdPower, OUTPUT);
    digitalWrite(kPinEpdPower, HIGH);
    delay(10);
  }
  SPI.begin(kPinEpdSck, kPinEpdMiso, kPinEpdMosi, kPinEpdCs);
  // 2 ms reset pulse is required by Waveshare/DESPI boards with the
  // "clever" reset circuit.
  display.init(0, true, 2, false);
  display.setRotation(0);
  display.setFullWindow();
}

template <typename DrawFn>
void showScreen(DrawFn draw) {
  displayBegin();
  display.firstPage();
  do {
    draw(display);
  } while (display.nextPage());
  display.hibernate();
  if (kPinEpdPower >= 0) digitalWrite(kPinEpdPower, LOW);
}

// ---- Network -------------------------------------------------------------

bool httpGet(const char* url, String& body) {
  WiFiClientSecure client;
  client.setInsecure();  // public, read-only data; skips CA bundle handling
  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  bool ok = code == HTTP_CODE_OK;
  if (ok) body = http.getString();
  http.end();
  return ok;
}

bool lookupZip(Settings& s) {
  char url[64];
  buildZipLookupUrl(url, sizeof url, s.zip);
  String body;
  if (!httpGet(url, body)) return false;
  return parseZipLookup(body.c_str(), body.length(), s.lat, s.lon, s.place, sizeof s.place);
}

bool fetchForecast(const Settings& s, Forecast& out) {
  char url[320];
  buildForecastUrl(url, sizeof url, s.lat, s.lon);
  for (int attempt = 0; attempt < 2; ++attempt) {
    String body;
    if (httpGet(url, body) && parseOpenMeteo(body.c_str(), body.length(), out)) return true;
    delay(1000);
  }
  return false;
}

bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();  // credentials saved by WiFiManager
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < kWifiTimeoutMs) delay(50);
  return WiFi.status() == WL_CONNECTED;
}

void wifiOff() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// Waits for a fresh NTP sync (not just a plausible RTC time).
bool syncTime(const char* tz) {
  sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
  configTzTime((tz && *tz) ? tz : "UTC0", "pool.ntp.org", "time.google.com");
  uint32_t start = millis();
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED &&
         millis() - start < kNtpTimeoutMs) {
    delay(50);
  }
  return sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
}

bool bootButtonHeld() {
  pinMode(kPinBootButton, INPUT_PULLUP);
  uint32_t start = millis();
  while (millis() - start < static_cast<uint32_t>(kSetupButtonHoldMs)) {
    if (digitalRead(kPinBootButton) == LOW) return true;
    delay(20);
  }
  return false;
}

// Runs the captive portal. Returns true once Wi-Fi works and the ZIP code
// resolves to a location.
bool runSetupPortal(Settings& s, int batteryPct) {
  // A fresh WPA2 password per session keeps the home Wi-Fi password encrypted
  // over the air while it's typed into the portal. esp_random() is a hardware
  // RNG once the radio is on.
  WiFi.mode(WIFI_STA);
  char apPassword[kSetupPasswordLength + 1];
  makeSetupPassword(esp_random, apPassword, sizeof apPassword);
  showScreen([&](Adafruit_GFX& g) {
    drawSetupScreen(g, kSetupApName, apPassword, "192.168.4.1", kSetupPortalMinutes);
  });

  WiFiManager wm;
  wm.setDebugOutput(false);
  wm.setTitle("Door Display");
  wm.setConfigPortalTimeout(kSetupPortalMinutes * 60);
  wm.setBreakAfterConfig(true);
  WiFiManagerParameter zipParam(
      "zip", "5-digit ZIP code", s.zip, 5,
      " inputmode='numeric' pattern='[0-9]{5}' required placeholder='e.g. 94110'");
  wm.addParameter(&zipParam);
  std::vector<const char*> menu = {"wifi", "exit"};
  wm.setMenu(menu);

  bool connected = wm.startConfigPortal(kSetupApName, apPassword);
  const char* zip = zipParam.getValue();
  if (isValidZip(zip)) snprintf(s.zip, sizeof s.zip, "%s", zip);

  if (!connected) connected = connectWifi();
  if (!connected) {
    showScreen([&](Adafruit_GFX& g) {
      drawMessageScreen(g, "Wi-Fi not set up",
                        "Setup timed out or the Wi-Fi password didn't work.\n"
                        "Press RESET, then hold BOOT to try again.",
                        batteryPct);
    });
    return false;
  }
  if (!isValidZip(s.zip) || !lookupZip(s)) {
    showScreen([&](Adafruit_GFX& g) {
      drawMessageScreen(g, "ZIP code not found",
                        "Wi-Fi is connected, but the ZIP code couldn't be looked up.\n"
                        "Press RESET, then hold BOOT and enter a valid 5-digit US ZIP code.",
                        batteryPct);
    });
    return false;
  }
  saveLocation(s);
  s.hasLocation = true;
  return true;
}

[[noreturn]] void sleepSeconds(double seconds) {
  wifiOff();
  esp_sleep_enable_timer_wakeup(correctedSleepMicros(seconds, rtc.sleepCorrection));
  rtc.programmedSleepS = seconds * rtc.sleepCorrection;
  esp_deep_sleep_start();
}

[[noreturn]] void sleepUntilReset() {
  wifiOff();
  rtc.sleepStartedAt = 0;
  esp_deep_sleep_start();  // no wake source: only RESET/power wakes it
}

void fillWeatherScreen(WeatherScreen& w, const Settings& s, const Forecast& f, int day,
                       const struct tm& shownDay, bool tomorrow, int fromHour,
                       time_t fetchedAt, bool stale, time_t next, int batteryPct) {
  memset(&w, 0, sizeof w);
  w.weekday = weekdayName(shownDay.tm_wday);
  formatIsoDate(shownDay, w.date, sizeof w.date);
  w.tomorrow = tomorrow;
  struct tm t;
  localtime_r(&fetchedAt, &t);
  formatClock(t, w.updatedAt, sizeof w.updatedAt);
  w.stale = stale;
  localtime_r(&next, &t);
  formatClock(t, w.nextUpdateAt, sizeof w.nextUpdateAt);
  snprintf(w.place, sizeof w.place, "%s %s", s.place, s.zip);
  const DayForecast& d = f.day[day];
  const bool f_units = kTemperatureUnit == 'F';
  float lowF = celsiusToFahrenheit(d.feelsLowC), highF = celsiusToFahrenheit(d.feelsHighC);
  w.feelsLow = f_units ? lowF : d.feelsLowC;
  w.feelsHigh = f_units ? highF : d.feelsHighC;
  w.unit = kTemperatureUnit;
  w.lowTier = tierForFeelsLikeF(lowF);
  w.highTier = tierForFeelsLikeF(highF);
  w.rain = rainOutlook(f, day, fromHour);
  w.batteryPercent = batteryPct;
}

}  // namespace

void setup() {
  const bool timerWake = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
  if (rtc.magic != kRtcMagic) {
    memset(&rtc, 0, sizeof rtc);
    rtc.magic = kRtcMagic;
    rtc.batteryPercent = -1;
    rtc.sleepCorrection = 1.0f;
  }

  // Measure before Wi-Fi loads the battery.
  int battery = readBatteryPercent();
  battery = timerWake ? smoothBatteryPercent(rtc.batteryPercent, battery) : battery;
  rtc.batteryPercent = battery;

  if (batteryLevel(battery) == BatteryLevel::Critical) {
    if (!rtc.criticalShown) {
      char last[48] = "";
      if (rtc.forecastFetchedAt > kMinValidEpoch) {
        struct tm t;
        localtime_r(&rtc.forecastFetchedAt, &t);
        char date[11], clock[12];
        formatIsoDate(t, date, sizeof date);
        formatClock(t, clock, sizeof clock);
        snprintf(last, sizeof last, "%s %s %s", weekdayName(t.tm_wday), date, clock);
      }
      showScreen([&](Adafruit_GFX& g) { drawCriticalBatteryScreen(g, battery, last); });
      rtc.criticalShown = true;
    }
    rtc.sleepStartedAt = 0;
    sleepSeconds(kCriticalRecheckMinutes * 60);
  }
  const bool recoveredFromCritical = rtc.criticalShown;
  rtc.criticalShown = false;

  Settings settings = loadSettings();
  applyTz(settings.tz);

  bool wantSetup = !settings.hasLocation;
  if (!timerWake && !wantSetup) wantSetup = bootButtonHeld();
  if (wantSetup) {
    if (!runSetupPortal(settings, battery)) sleepUntilReset();
  }

  bool online = WiFi.status() == WL_CONNECTED || connectWifi();
  if (!online && !timerWake) {
    // Manual reset and the network is unreachable: offer setup right away.
    if (!runSetupPortal(settings, battery)) sleepUntilReset();
    online = true;
  }

  bool synced = online && syncTime(settings.tz);
  time_t now = time(nullptr);
  if (synced && timerWake && rtc.sleepStartedAt > 0) {
    double actual = difftime(now, rtc.sleepStartedAt) - millis() / 1000.0;
    rtc.sleepCorrection =
        updateSleepCorrection(rtc.sleepCorrection, rtc.programmedSleepS, actual);
  }

  bool fetched = false;
  if (online) {
    Forecast f;
    if (fetchForecast(settings, f)) {
      rtc.forecast = f;
      rtc.forecastFetchedAt = now;
      fetched = true;
      char tz[48];
      posixTzForIana(f.timezone, f.utcOffsetSeconds, tz, sizeof tz);
      if (strcmp(tz, settings.tz) != 0) {
        snprintf(settings.tz, sizeof settings.tz, "%s", tz);
        saveTz(tz);
        applyTz(tz);
      }
    }
  }
  wifiOff();

  RefreshWindow window;
  window.startHour = kRefreshStartHour;
  window.endHour = kRefreshEndHour;
  struct tm local;
  localtime_r(&now, &local);
  time_t next = nextRefreshTime(now, window);

  const bool clockValid = now > kMinValidEpoch;
  const bool tomorrow = clockValid && forecastDayOffset(local, kShowTomorrowFromHour) == 1;
  struct tm shown = local;
  if (tomorrow) {
    time_t t = now + 24 * 3600;
    localtime_r(&t, &shown);
  }
  char wanted[11];
  formatIsoDate(shown, wanted, sizeof wanted);
  int day = clockValid ? forecastDayIndex(rtc.forecast, wanted) : -1;

  if (day >= 0) {
    WeatherScreen w;
    fillWeatherScreen(w, settings, rtc.forecast, day, shown, tomorrow,
                      tomorrow ? 0 : local.tm_hour, rtc.forecastFetchedAt, !fetched, next,
                      battery);
    showScreen([&](Adafruit_GFX& g) { drawWeatherScreen(g, w); });
  } else if (!timerWake || recoveredFromCritical || fetched || rtc.forecast.valid) {
    char body[256], when[12];
    struct tm nt;
    localtime_r(&next, &nt);
    formatClock(nt, when, sizeof when);
    snprintf(body, sizeof body,
             "%s\nWill try again at %s. To change Wi-Fi or ZIP code: press RESET, then "
             "hold BOOT.",
             !online ? "Couldn't connect to Wi-Fi." : "Couldn't download today's forecast.",
             when);
    showScreen([&](Adafruit_GFX& g) { drawMessageScreen(g, "No weather data", body, battery); });
    rtc.forecast.valid = false;  // don't redraw the same error every hour
  }

  rtc.sleepStartedAt = synced ? time(nullptr) : 0;
  sleepSeconds(difftime(next, time(nullptr)));
}

void loop() {}
