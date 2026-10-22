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
using PortalArg = std::function<std::string(const char*)>;

// The reminders web page, independent of any HTTP server: feed it each
// request and send back the response. Access needs the one-time PIN shown on
// the display; after that a random session token protects every action.
class ReminderPortal {
 public:
  ReminderPortal(ReminderList* list, uint32_t (*random32)());
  void setToday(const char* isoDate, int weekday);
  PortalResponse handle(const char* method, const char* path, const PortalArg& arg);

  const char* pin() const { return st_.pin; }
  const char* token() const { return st_.token; }
  int failedPins() const { return st_.failedPins; }
  bool done() const { return st_.done; }
  bool wifiSetupRequested() const { return st_.wifiSetupRequested; }
  // True once after the reminder list changed (so the caller can save it).
  bool takeChanged();

 private:
  static void init(PortalState& st, ReminderList* list, uint32_t (*random32)());
  static PortalResponse dispatch(PortalState& st, const char* method, const char* path,
                                 const PortalArg& arg);
  PortalState st_;
};

std::string htmlEscape(const std::string& s);

}  // namespace door
