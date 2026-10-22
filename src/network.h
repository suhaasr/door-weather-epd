#pragma once
// Wi-Fi station mode and NTP. Credentials are the ones saved by the setup portal.

namespace door {

class Network {
 public:
  bool connect();
  bool connected() const;
  void off();
  // Waits for a fresh NTP sync (not just a plausible RTC time).
  bool syncTime(const char* tz);
  static void applyTimeZone(const char* tz);

 private:
  static constexpr unsigned long kWifiTimeoutMs = 15000;
  static constexpr unsigned long kNtpTimeoutMs = 10000;
};

}  // namespace door
