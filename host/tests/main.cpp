// Runs every registered test suite, or those whose name contains argv[1].
#include "test.h"

namespace test {
int checks = 0;
int failures = 0;
std::vector<Case>& registry() {
  static std::vector<Case> cases;
  return cases;
}
}  // namespace test

int main(int argc, char** argv) {
  const char* filter = argc > 1 ? argv[1] : "";
  int ran = 0;
  for (const test::Case& c : test::registry()) {
    if (*filter && !strstr(c.name, filter)) continue;
    const int before = test::failures;
    c.fn();
    ++ran;
    if (test::failures != before) fprintf(stderr, "FAILED: %s\n", c.name);
  }
  printf("%d suites, %d checks, %d failures\n", ran, test::checks, test::failures);
  return test::failures || !ran ? 1 : 0;
}
