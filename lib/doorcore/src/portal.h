#pragma once
#include <stdint.h>

#include <functional>
#include <string>

#include "reminders.h"

namespace door {

constexpr int kPortalMaxPinAttempts = 5;

// State of one reminders-page session. The device shows `pin` on the e-ink
// screen; entering it on the phone yields `token`, which every later request
// must carry.
struct PortalState {
  char pin[7];
  char token[17];
  int failedPins;
  bool done;                // user tapped Done, or too many wrong PINs
  bool wifiSetupRequested;  // user asked for the Wi-Fi/ZIP hotspot
  bool changed;             // reminders were modified; caller should persist them
  ReminderList* list;
  char today[11];  // "YYYY-MM-DD", or "" if the clock isn't set
  int todayWeekday;
};

struct PortalResponse {
  int status = 200;
  std::string location;  // for 303 redirects
  std::string body;
};

// `random32` must be a cryptographic RNG (esp_random() with the radio on).
void initPortal(PortalState& st, ReminderList* list, uint32_t (*random32)());

// Handles one request. `arg(name)` returns a query/form parameter or "".
PortalResponse handlePortalRequest(PortalState& st, const char* method, const char* path,
                                   const std::function<std::string(const char*)>& arg);

std::string htmlEscape(const std::string& s);

}  // namespace door
