#pragma once
#include <stddef.h>
#include <stdint.h>

namespace door {

constexpr int kSetupPasswordLength = 10;
// Lowercase letters and digits that can't be confused on screen (no 0/o, 1/l/i).
constexpr const char* kSetupPasswordAlphabet = "abcdefghjkmnpqrstuvwxyz23456789";

// Fills `out` with a random WPA2 passphrase for the setup hotspot (~49 bits).
// `random32` must be a cryptographic RNG (esp_random() with the radio on).
void makeSetupPassword(uint32_t (*random32)(), char* out, size_t len);

}  // namespace door
