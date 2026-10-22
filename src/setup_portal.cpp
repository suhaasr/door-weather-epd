#include "setup_portal.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "config.h"
#include "forecast.h"
#include "screen.h"
#include "setup_password.h"
#include "weather_client.h"

namespace door {

bool SetupPortal::bootButtonHeld() {
  pinMode(kPinBootButton, INPUT_PULLUP);
  uint32_t start = millis();
  while (millis() - start < static_cast<uint32_t>(kSetupButtonHoldMs)) {
    if (digitalRead(kPinBootButton) == LOW) return true;
    delay(20);
  }
  return false;
}

bool SetupPortal::run(Settings& s, int batteryPct) {
  // A fresh WPA2 password per session keeps the home Wi-Fi password encrypted
  // over the air while it's typed into the portal. esp_random() is a hardware
  // RNG once the radio is on.
  WiFi.mode(WIFI_STA);
  char apPassword[kSetupPasswordLength + 1];
  makeSetupPassword(esp_random, apPassword, sizeof apPassword);
  display_.show([&](Adafruit_GFX& g) {
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

  if (!connected) connected = network_.connect();
  if (!connected) {
    display_.show([&](Adafruit_GFX& g) {
      drawMessageScreen(g, "Wi-Fi not set up",
                        "Setup timed out or the Wi-Fi password didn't work.\n"
                        "Press RESET, then hold BOOT to try again.",
                        batteryPct);
    });
    return false;
  }
  if (!isValidZip(s.zip) || !WeatherClient::lookupZip(s.zip, s.lat, s.lon, s.place, sizeof s.place)) {
    display_.show([&](Adafruit_GFX& g) {
      drawMessageScreen(g, "ZIP code not found",
                        "Wi-Fi is connected, but the ZIP code couldn't be looked up.\n"
                        "Press RESET, then hold BOOT and enter a valid 5-digit US ZIP code.",
                        batteryPct);
    });
    return false;
  }
  store_.saveLocation(s);
  s.hasLocation = true;
  return true;}

}  // namespace door
