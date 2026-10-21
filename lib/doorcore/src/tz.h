#pragma once
#include <stddef.h>

namespace door {

// Converts the IANA zone that Open-Meteo reports (e.g. "America/New_York")
// into a POSIX TZ string with DST rules. Unknown zones fall back to a fixed
// offset built from utcOffsetSeconds.
void posixTzForIana(const char* iana, long utcOffsetSeconds, char* buf,
                    size_t len);

}  // namespace door
