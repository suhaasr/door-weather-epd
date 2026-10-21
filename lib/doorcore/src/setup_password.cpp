#include "setup_password.h"

#include <string.h>

namespace door {

void makeSetupPassword(uint32_t (*random32)(), char* out, size_t len) {
  if (len == 0) return;
  const uint32_t n = static_cast<uint32_t>(strlen(kSetupPasswordAlphabet));
  const uint32_t limit = UINT32_MAX - UINT32_MAX % n;  // rejection sampling: no modulo bias
  size_t i = 0;
  for (; i < static_cast<size_t>(kSetupPasswordLength) && i + 1 < len; ++i) {
    uint32_t r;
    do r = random32(); while (r >= limit);
    out[i] = kSetupPasswordAlphabet[r % n];
  }
  out[i] = '\0';
}

}  // namespace door
