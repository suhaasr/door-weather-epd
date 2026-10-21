// Minimal Arduino shim so Adafruit GFX can be compiled on the host for
// tests and screen previews.
#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGMEM
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char*)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const unsigned short*)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const unsigned long*)(addr))
#endif

#ifndef radians
#define radians(deg) ((deg) * 0.017453292519943295)
#endif
#ifndef degrees
#define degrees(rad) ((rad) * 57.29577951308232)
#endif

typedef bool boolean;
typedef uint8_t byte;

#ifdef __cplusplus
#include "Print.h"
#endif
