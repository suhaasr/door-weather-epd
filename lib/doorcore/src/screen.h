#pragma once
#include <Adafruit_GFX.h>

#include "attire.h"
#include "forecast.h"
#include "reminders.h"

namespace door {

constexpr int kScreenW = 800;
constexpr int kScreenH = 480;

struct ScreenAlert {
  char event[40];  // NWS event, e.g. "Winter Storm Warning"
  char until[24];  // "until Sat 4 PM"
};

struct WeatherScreen {
  const char* weekday;    // "Wednesday"
  char date[11];          // "2026-10-21"
  bool tomorrow;          // forecast shown is for tomorrow
  char updatedAt[12];     // "7:00 AM" (time of the forecast fetch)
  bool stale;             // latest fetch failed; showing an older forecast
  char nextUpdateAt[12];  // "8:00 AM"
  char place[48];         // "San Francisco, CA 94110"
  float feelsLow;         // already in the display unit
  float feelsHigh;
  char unit;              // 'F' or 'C'
  Tier lowTier;
  Tier highTier;
  RainOutlook rain;
  int batteryPercent;
  const char* reminders[kMaxReminders];  // reminders for the day shown
  int reminderCount;
  int8_t hourlyRain[24];  // precipitation chance per local hour of the day shown
  int rainFromHour;       // earlier hours are already past
  ScreenAlert alerts[kMaxAlerts];  // in effect on the day shown, most severe first
  int alertCount;
  float uvMax;            // < 0 if unknown
  float gustMph;          // < 0 if unknown
  int aqi;                // US AQI, < 0 if unknown
  int batteryDaysLeft;    // <= 0 if unknown
};

void drawWeatherScreen(Adafruit_GFX& g, const WeatherScreen& s);

// Full-screen "recharge me" banner shown at <= 1% battery. lastUpdate may be
// empty.
void drawCriticalBatteryScreen(Adafruit_GFX& g, int batteryPercent,
                               const char* lastUpdate);

// Instructions shown while the Wi-Fi setup portal is running.
void drawSetupScreen(Adafruit_GFX& g, const char* apName, const char* apPassword,
                     const char* portalIp,
                     int portalMinutes);

// Shown while the reminders page is served on the home network.
void drawRemindersScreen(Adafruit_GFX& g, const char* url, const char* altUrl, const char* pin,
                         int idleMinutes);

// Generic title + body message (errors, setup results).
void drawMessageScreen(Adafruit_GFX& g, const char* title, const char* body,
                       int batteryPercent);

// "2 PM" / "12 AM" / "midnight" for hour 24.
void formatHour(int hour, char* buf, int len);

}  // namespace door
