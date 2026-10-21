// Tiny dependency-free PNG writer for 1-bit images (uncompressed deflate).
#pragma once
#include <stdint.h>
#include <stdio.h>

#include <vector>

inline uint32_t pngCrc(const uint8_t* d, size_t n, uint32_t c = 0xFFFFFFFFu) {
  for (size_t i = 0; i < n; ++i) {
    c ^= d[i];
    for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return c;
}

inline void pngPut32(std::vector<uint8_t>& v, uint32_t x) {
  v.push_back(x >> 24); v.push_back(x >> 16); v.push_back(x >> 8); v.push_back(x);
}

inline void pngChunk(FILE* f, const char* type, const std::vector<uint8_t>& data) {
  std::vector<uint8_t> buf;
  pngPut32(buf, static_cast<uint32_t>(data.size()));
  buf.insert(buf.end(), type, type + 4);
  buf.insert(buf.end(), data.begin(), data.end());
  uint32_t crc = pngCrc(buf.data() + 4, buf.size() - 4) ^ 0xFFFFFFFFu;
  pngPut32(buf, crc);
  fwrite(buf.data(), 1, buf.size(), f);
}

// rows: MSB-first packed bits, 1 = white.
inline bool writePng1(const char* path, const uint8_t* bits, int w, int h) {
  FILE* f = fopen(path, "wb");
  if (!f) return false;
  static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
  fwrite(sig, 1, 8, f);
  std::vector<uint8_t> ihdr;
  pngPut32(ihdr, w); pngPut32(ihdr, h);
  ihdr.push_back(1); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
  pngChunk(f, "IHDR", ihdr);

  const int rowBytes = (w + 7) / 8;
  std::vector<uint8_t> raw;
  for (int y = 0; y < h; ++y) {
    raw.push_back(0);  // filter: none
    raw.insert(raw.end(), bits + y * rowBytes, bits + (y + 1) * rowBytes);
  }
  std::vector<uint8_t> z = {0x78, 0x01};
  size_t pos = 0;
  while (pos < raw.size() || raw.empty()) {
    size_t n = raw.size() - pos;
    if (n > 65535) n = 65535;
    bool last = pos + n == raw.size();
    z.push_back(last ? 1 : 0);
    z.push_back(n & 0xFF); z.push_back(n >> 8);
    z.push_back(~n & 0xFF); z.push_back((~n >> 8) & 0xFF);
    z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
    pos += n;
    if (raw.empty()) break;
  }
  uint32_t a = 1, b = 0;
  for (uint8_t c : raw) { a = (a + c) % 65521; b = (b + a) % 65521; }
  pngPut32(z, (b << 16) | a);
  pngChunk(f, "IDAT", z);
  pngChunk(f, "IEND", {});
  fclose(f);
  return true;
}
