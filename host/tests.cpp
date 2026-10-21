// Host unit tests for the hardware-independent core.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstream>
#include <sstream>
#include <string>

#include "attire.h"
#include "battery.h"
#include "drift.h"
#include "forecast.h"
#include "schedule.h"
#include "screen.h"
#include "setup_password.h"
#include "tz.h"

#include <Adafruit_GFX.h>

using namespace door;

static int failures = 0;
static int checks = 0;
#define CHECK(cond)                                                     \
  do {                                                                  \
    ++checks;                                                           \
    if (!(cond)) {                                                      \
      ++failures;                                                       \
      fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
    }                                                                   \
  } while (0)

static std::string readFile(const char* path) {
  std::ifstream in(path);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static void useTz(const char* tz) {
  setenv("TZ", tz, 1);
  tzset();
}

static time_t localTime(int y, int mo, int d, int h, int mi, int s = 0) {
  struct tm t = {};
  t.tm_year = y - 1900;
  t.tm_mon = mo - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = mi;
  t.tm_sec = s;
  t.tm_isdst = -1;
  return mktime(&t);
}

static struct tm local(time_t t) {
  struct tm r;
  localtime_r(&t, &r);
  return r;
}

static void testBattery() {
  CHECK(batteryPercentFromMillivolts(4250) == 100);
  CHECK(batteryPercentFromMillivolts(4200) == 100);
  CHECK(batteryPercentFromMillivolts(3820) == 50);
  CHECK(batteryPercentFromMillivolts(3600) == 5);
  CHECK(batteryPercentFromMillivolts(3599) == 4);
  CHECK(batteryPercentFromMillivolts(3450) == 1);
  CHECK(batteryPercentFromMillivolts(3300) == 0);
  CHECK(batteryPercentFromMillivolts(3000) == 0);
  // Monotonic over the whole range.
  int prev = 0;
  for (int mv = 3000; mv <= 4300; mv += 5) {
    int p = batteryPercentFromMillivolts(mv);
    CHECK(p >= prev);
    prev = p;
  }
  CHECK(batteryLevel(100) == BatteryLevel::Ok);
  CHECK(batteryLevel(5) == BatteryLevel::Ok);
  CHECK(batteryLevel(4) == BatteryLevel::Low);
  CHECK(batteryLevel(2) == BatteryLevel::Low);
  CHECK(batteryLevel(1) == BatteryLevel::Critical);
  CHECK(batteryLevel(0) == BatteryLevel::Critical);

  CHECK(smoothBatteryPercent(-1, 42) == 42);
  CHECK(smoothBatteryPercent(40, 43) == 40);  // noise up is ignored
  CHECK(smoothBatteryPercent(40, 38) == 38);
  CHECK(smoothBatteryPercent(3, 95) == 95);   // recharged
}

static void testSchedule() {
  useTz("EST5EDT,M3.2.0,M11.1.0");
  RefreshWindow w;  // 3..10
  struct tm t;

  // 2:30 AM -> 3:00 AM same day.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 2, 30), w));
  CHECK(t.tm_mday == 21 && t.tm_hour == 3 && t.tm_min == 0);

  // 3:00:20 AM (just refreshed) -> 4:00 AM.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 3, 0, 20), w));
  CHECK(t.tm_mday == 21 && t.tm_hour == 4 && t.tm_min == 0);

  // Woke early at 3:57 because of drift -> next slot is 5:00, not 4:00.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 3, 57), w));
  CHECK(t.tm_hour == 5);

  // 9:00 -> 10:00 (10 AM is the last refresh).
  t = local(nextRefreshTime(localTime(2026, 10, 21, 9, 0, 30), w));
  CHECK(t.tm_mday == 21 && t.tm_hour == 10);

  // 10:00 -> 3:00 next day.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 10, 0, 30), w));
  CHECK(t.tm_mday == 22 && t.tm_hour == 3);

  // Evening setup -> 3:00 next day.
  t = local(nextRefreshTime(localTime(2026, 10, 21, 19, 45), w));
  CHECK(t.tm_mday == 22 && t.tm_hour == 3);

  // Every refresh in a day: exactly 3,4,...,10.
  time_t now = localTime(2026, 10, 21, 0, 0);
  int count = 0;
  for (time_t n = nextRefreshTime(now, w); n < localTime(2026, 10, 22, 0, 0);
       n = nextRefreshTime(n + 20, w)) {
    t = local(n);
    CHECK(t.tm_hour == 3 + count && t.tm_min == 0);
    ++count;
  }
  CHECK(count == 8);

  // DST starts 2026-03-08 at 2 AM: still 3:00 local.
  t = local(nextRefreshTime(localTime(2026, 3, 7, 11, 0), w));
  CHECK(t.tm_mday == 8 && t.tm_hour == 3 && t.tm_min == 0);
  // DST ends 2026-11-01.
  t = local(nextRefreshTime(localTime(2026, 10, 31, 11, 0), w));
  CHECK(t.tm_mday == 1 && t.tm_hour == 3 && t.tm_min == 0);

  // Window wrapping midnight (22..2).
  RefreshWindow wrap{22, 2};
  t = local(nextRefreshTime(localTime(2026, 10, 21, 12, 0), wrap));
  CHECK(t.tm_hour == 22);
  t = local(nextRefreshTime(localTime(2026, 10, 21, 23, 30), wrap));
  CHECK(t.tm_hour == 0 && t.tm_mday == 22);

  struct tm m = local(localTime(2026, 10, 21, 7, 0));
  CHECK(forecastDayOffset(m, 18) == 0);
  m = local(localTime(2026, 10, 21, 18, 0));
  CHECK(forecastDayOffset(m, 18) == 1);

  char buf[16];
  formatClock(local(localTime(2026, 10, 21, 0, 5)), buf, sizeof buf);
  CHECK(strcmp(buf, "12:05 AM") == 0);
  formatClock(local(localTime(2026, 10, 21, 13, 0)), buf, sizeof buf);
  CHECK(strcmp(buf, "1:00 PM") == 0);
  formatIsoDate(local(localTime(2026, 1, 2, 7, 0)), buf, sizeof buf);
  CHECK(strcmp(buf, "2026-01-02") == 0);
  CHECK(strcmp(weekdayName(local(localTime(2026, 10, 21, 7, 0)).tm_wday), "Wednesday") == 0);

  formatHour(0, buf, sizeof buf);
  CHECK(strcmp(buf, "12 AM") == 0);
  formatHour(12, buf, sizeof buf);
  CHECK(strcmp(buf, "12 PM") == 0);
  formatHour(14, buf, sizeof buf);
  CHECK(strcmp(buf, "2 PM") == 0);
  formatHour(24, buf, sizeof buf);
  CHECK(strcmp(buf, "midnight") == 0);
}

static void testDrift() {
  // RTC runs 5% fast: we programmed 10000 s but only 9524 s really passed.
  float c = updateSleepCorrection(1.0f, 10000, 10000 / 1.05);
  CHECK(c > 1.02f && c < 1.03f);
  for (int i = 0; i < 10; ++i) {
    double desired = 10000;
    double programmed = desired * c;
    c = updateSleepCorrection(c, programmed, programmed / 1.05);
  }
  CHECK(c > 1.045f && c < 1.055f);
  CHECK(updateSleepCorrection(1.0f, 600, 500) == 1.0f);       // too short to learn from
  CHECK(updateSleepCorrection(1.0f, 10000, 2000) == 1.0f);    // implausible
  CHECK(updateSleepCorrection(NAN, 10000, 10000) == 1.0f);    // garbage state
  CHECK(correctedSleepMicros(3600, 1.0f) == 3600000000ULL);
  CHECK(llabs((long long)correctedSleepMicros(100, 5.0f) - 115000000LL) < 10);  // clamped
}

static void testTz() {
  char buf[48];
  posixTzForIana("America/New_York", -14400, buf, sizeof buf);
  CHECK(strcmp(buf, "EST5EDT,M3.2.0,M11.1.0") == 0);
  posixTzForIana("America/Los_Angeles", -25200, buf, sizeof buf);
  CHECK(strcmp(buf, "PST8PDT,M3.2.0,M11.1.0") == 0);
  posixTzForIana("America/Indiana/Knox", -18000, buf, sizeof buf);
  CHECK(strcmp(buf, "CST6CDT,M3.2.0,M11.1.0") == 0);
  posixTzForIana("America/Indiana/Indianapolis", -14400, buf, sizeof buf);
  CHECK(strcmp(buf, "EST5EDT,M3.2.0,M11.1.0") == 0);
  posixTzForIana("America/Phoenix", -25200, buf, sizeof buf);
  CHECK(strcmp(buf, "MST7") == 0);
  posixTzForIana("Europe/Somewhere", 3600, buf, sizeof buf);
  CHECK(strcmp(buf, "UTC-1:00") == 0);
  posixTzForIana(nullptr, -19800, buf, sizeof buf);
  CHECK(strcmp(buf, "UTC+5:30") == 0);
}

static void testAttire() {
  CHECK(tierForFeelsLikeF(10) == Tier::Freezing);
  CHECK(tierForFeelsLikeF(31.9f) == Tier::Freezing);
  CHECK(tierForFeelsLikeF(32) == Tier::Cold);
  CHECK(tierForFeelsLikeF(50) == Tier::Chilly);
  CHECK(tierForFeelsLikeF(60) == Tier::Cool);
  CHECK(tierForFeelsLikeF(70) == Tier::Mild);
  CHECK(tierForFeelsLikeF(80) == Tier::Warm);
  CHECK(tierForFeelsLikeF(85) == Tier::Hot);
  Outfit o = outfitFor(Tier::Freezing, Gender::Woman);
  CHECK(o.top == Top::LongCoat && o.gloves && o.beanie && o.scarf && o.shoes == Shoes::Boots);
  o = outfitFor(Tier::Hot, Gender::Man);
  CHECK(o.bottom == Bottom::Shorts && o.sunglasses);
  CHECK(celsiusToFahrenheit(100) == 212);
}

static void testForecast() {
  std::string json = readFile("fixtures/open_meteo_sf.json");
  CHECK(!json.empty());
  Forecast f;
  CHECK(parseOpenMeteo(json.data(), json.size(), f));
  CHECK(f.valid);
  CHECK(strcmp(f.timezone, "America/Los_Angeles") == 0);
  CHECK(f.utcOffsetSeconds == -25200);
  CHECK(strcmp(f.day[0].date, "2026-10-21") == 0);
  CHECK(strcmp(f.day[1].date, "2026-10-22") == 0);
  CHECK(f.day[0].feelsHighC > 22.5f && f.day[0].feelsHighC < 22.7f);
  CHECK(f.day[0].feelsLowC > 12.4f && f.day[0].feelsLowC < 12.6f);
  CHECK(f.day[0].precipMaxPct == 31);
  CHECK(f.hourlyPrecipPct[17] == 31);
  CHECK(forecastDayIndex(f, "2026-10-22") == 1);
  CHECK(forecastDayIndex(f, "2026-10-23") == -1);

  // Fixture: 15:00 = 14%, 16 = 26, 17 = 31, 18 = 16, 19 = 10, 20 = 10, 21 = 7.
  RainOutlook r = rainOutlook(f, 0, 7);
  CHECK(r.likely && r.maxPct == 31 && r.firstHour == 15 && r.lastHour == 20 && !r.snow);
  r = rainOutlook(f, 0, 21);
  CHECK(!r.likely && r.maxPct == 7 && r.firstHour == -1);
  r = rainOutlook(f, 0, 7, 50);
  CHECK(!r.likely);

  // Exactly 10% counts as likely.
  Forecast g = f;
  for (int h = 0; h < kForecastHours; ++h) g.hourlyPrecipPct[h] = 0;
  g.hourlyPrecipPct[24 + 8] = 10;
  g.hourlyWeatherCode[24 + 8] = 73;  // snow
  r = rainOutlook(g, 1, 0);
  CHECK(r.likely && r.firstHour == 8 && r.lastHour == 8 && r.snow);
  g.hourlyPrecipPct[24 + 8] = 9;
  CHECK(!rainOutlook(g, 1, 0).likely);

  CHECK(!parseOpenMeteo("{}", 2, f));
  CHECK(!parseOpenMeteo("not json", 8, f));

  char url[320];
  buildForecastUrl(url, sizeof url, 37.75f, -122.4153f);
  CHECK(strstr(url, "latitude=37.7500&longitude=-122.4153") != nullptr);
  CHECK(strstr(url, "apparent_temperature_max") != nullptr);
  CHECK(strstr(url, "forecast_days=2") != nullptr);
}

static void testZip() {
  CHECK(isValidZip("94110"));
  CHECK(isValidZip("02134"));
  CHECK(!isValidZip("9411"));
  CHECK(!isValidZip("941101"));
  CHECK(!isValidZip("94a10"));
  CHECK(!isValidZip(nullptr));
  std::string json = readFile("fixtures/zippopotam_94110.json");
  float lat = 0, lon = 0;
  char place[40];
  CHECK(parseZipLookup(json.data(), json.size(), lat, lon, place, sizeof place));
  CHECK(lat > 37.75f && lat < 37.76f);
  CHECK(lon < -122.41f && lon > -122.42f);
  CHECK(strcmp(place, "San Francisco, CA") == 0);
  CHECK(!parseZipLookup("{}", 2, lat, lon, place, sizeof place));
  char url[64];
  buildZipLookupUrl(url, sizeof url, "94110");
  CHECK(strcmp(url, "https://api.zippopotam.us/us/94110") == 0);
}

static int blackPixels(GFXcanvas1& c, int x0, int y0, int x1, int y1) {
  int n = 0;
  for (int y = y0; y < y1; ++y)
    for (int x = x0; x < x1; ++x) n += c.getPixel(x, y) ? 0 : 1;
  return n;
}

static void testRendering() {
  GFXcanvas1 canvas(kScreenW, kScreenH);
  WeatherScreen s = {};
  s.weekday = "Wednesday";
  strcpy(s.date, "2026-10-21");
  strcpy(s.updatedAt, "7:00 AM");
  strcpy(s.nextUpdateAt, "8:00 AM");
  strcpy(s.place, "San Francisco, CA 94110");
  s.feelsLow = 41;
  s.feelsHigh = 55;
  s.unit = 'F';
  s.lowTier = Tier::Cold;
  s.highTier = Tier::Cool;
  s.rain = {true, false, 60, 14, 18};
  s.batteryPercent = 80;

  drawWeatherScreen(canvas, s);
  // Rain banner is solid black; bottom strip is mostly white.
  int bannerArea = (kScreenW - 40) * 80;
  CHECK(blackPixels(canvas, 20, 70, kScreenW - 20, 150) > bannerArea / 2);
  CHECK(blackPixels(canvas, 0, 448, kScreenW, kScreenH) < kScreenW * 32 / 4);

  s.rain = {false, false, 4, -1, -1};
  drawWeatherScreen(canvas, s);
  CHECK(blackPixels(canvas, 20, 70, kScreenW - 20, 150) < bannerArea / 4);

  s.batteryPercent = 4;  // low: black banner along the bottom
  drawWeatherScreen(canvas, s);
  CHECK(blackPixels(canvas, 0, 410, kScreenW, kScreenH) > kScreenW * 70 * 3 / 4);
  s.batteryPercent = 5;  // not low yet
  drawWeatherScreen(canvas, s);
  CHECK(blackPixels(canvas, 0, 448, kScreenW, kScreenH) < kScreenW * 32 / 4);

  drawCriticalBatteryScreen(canvas, 1, "Wednesday 2026-10-21 7:00 AM");
  CHECK(blackPixels(canvas, 0, 0, kScreenW, kScreenH) > kScreenW * kScreenH * 3 / 4);
}

static uint32_t seqState;
static uint32_t seqRandom() { return seqState++ * 2654435761u; }
static int maxCalls;
static uint32_t mostlyMaxRandom() { return maxCalls++ < 3 ? UINT32_MAX : 7; }

static void testSetupPassword() {
  char pw[kSetupPasswordLength + 1];
  seqState = 1;
  makeSetupPassword(seqRandom, pw, sizeof pw);
  CHECK(strlen(pw) == static_cast<size_t>(kSetupPasswordLength));
  CHECK(strlen(pw) >= 8);  // WPA2 minimum
  for (const char* c = pw; *c; ++c) CHECK(strchr(kSetupPasswordAlphabet, *c) != nullptr);
  CHECK(strpbrk(kSetupPasswordAlphabet, "01ilo") == nullptr);
  char other[kSetupPasswordLength + 1];
  makeSetupPassword(seqRandom, other, sizeof other);
  CHECK(strcmp(pw, other) != 0);
  // Values in the biased tail are rejected and redrawn.
  maxCalls = 0;
  makeSetupPassword(mostlyMaxRandom, pw, sizeof pw);
  CHECK(maxCalls == 3 + kSetupPasswordLength);
  CHECK(pw[0] == kSetupPasswordAlphabet[7]);
  // Never overruns a short buffer.
  char tiny[4];
  makeSetupPassword(seqRandom, tiny, sizeof tiny);
  CHECK(strlen(tiny) == 3);
}

int main() {
  testBattery();
  testSchedule();
  testDrift();
  testTz();
  testAttire();
  testForecast();
  testZip();
  testRendering();
  testSetupPassword();
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
