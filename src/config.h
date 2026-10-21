#pragma once
// User-tunable settings. Wi-Fi and ZIP code are entered through the setup
// portal and stored on the device, not here.

// Refresh at the top of every hour from 3 AM through 10 AM (local time).
constexpr int kRefreshStartHour = 3;
constexpr int kRefreshEndHour = 10;

// From this hour on, show tomorrow's forecast (e.g. right after evening setup).
constexpr int kShowTomorrowFromHour = 18;

// 'F' or 'C' for the feels-like temperatures.
constexpr char kTemperatureUnit = 'F';

// Setup portal.
constexpr const char* kSetupApName = "DoorDisplay-Setup";
constexpr int kSetupPortalMinutes = 10;
constexpr int kRemindersIdleMinutes = 10;  // reminders page closes after this idle time
constexpr const char* kMdnsName = "doordisplay";  // http://doordisplay.local
constexpr int kSetupButtonHoldMs = 3000;  // window after RESET to press BOOT

// While the battery is critically low, check for charging this often.
constexpr int kCriticalRecheckMinutes = 30;

// Calibration: added to the measured battery voltage (mV).
constexpr int kBatteryMillivoltOffset = 0;

// Pins: FireBeetle 2 ESP32-E wired to a Waveshare 7.5" V2 panel through a
// DESPI-C02 (or Waveshare e-Paper HAT) adapter. See README for wiring.
constexpr int kPinBatteryAdc = 34;  // A2, on-board 1:2 divider from BAT+
constexpr float kBatteryDividerRatio = 2.0f;
constexpr int kPinBootButton = 0;
constexpr int kPinEpdBusy = 14;
constexpr int kPinEpdCs = 13;
constexpr int kPinEpdRst = 21;
constexpr int kPinEpdDc = 22;
constexpr int kPinEpdSck = 18;
constexpr int kPinEpdMiso = 19;  // not connected on the panel
constexpr int kPinEpdMosi = 23;
constexpr int kPinEpdPower = -1;  // set to 26 if the adapter has a PWR pin

// Waveshare 7.5" V2 = GxEPD2_750_T7. Newer GDEY075T7 panels use
// GxEPD2_750_GDEY075T7 instead.
#define DOOR_EPD_DRIVER GxEPD2_750_T7
