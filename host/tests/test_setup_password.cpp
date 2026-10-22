#include "test.h"

#include <math.h>

#include <map>

#include "setup_password.h"

using namespace door;
using namespace test;

static uint32_t seqState;
static uint32_t seqRandom() { return seqState++ * 2654435761u; }
static int maxCalls;
static uint32_t mostlyMaxRandom() { return maxCalls++ < 3 ? UINT32_MAX : 7; }

TEST(setup_password) {
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

