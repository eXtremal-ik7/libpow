#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <source_location>
#include <string>

inline std::string toHex(const uint8_t *data, size_t size)
{
  static const char digits[] = "0123456789abcdef";
  std::string result;
  for (size_t i = 0; i < size; i++) {
    result.push_back(digits[data[i] >> 4]);
    result.push_back(digits[data[i] & 0x0F]);
  }
  return result;
}

// Hex to bytes for constants written out in the tests; the input is trusted
inline void fromHex(const char *hex, uint8_t *out, size_t size)
{
  auto digit = [](char c) -> uint8_t {
    return static_cast<uint8_t>(c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10);
  };
  for (size_t i = 0; i < size; i++)
    out[i] = static_cast<uint8_t>(digit(hex[2 * i]) << 4 | digit(hex[2 * i + 1]));
}

// The caller's line comes in through the default argument, so a failure points
// at the check that made it and not at this function
inline bool checkHash(const uint8_t *hash,
                      size_t hashSize,
                      const char *expected,
                      const char *what,
                      size_t messageSize,
                      const std::source_location location = std::source_location::current())
{
  const std::string got = toHex(hash, hashSize);
  if (got != expected) {
    fprintf(stderr,
            " * %s:%u %s size=%zu mismatch got %s should be %s\n",
            location.file_name(),
            static_cast<unsigned>(location.line()),
            what,
            messageSize,
            got.c_str(),
            expected);
    return false;
  }

  return true;
}

// Splitmix64 from a fixed seed as little endian bytes: the vector generators
// produce the same stream, so a table row and the message it was made from agree
inline void fillTestData(uint8_t *data, size_t size, uint64_t seed)
{
  uint64_t state = seed;
  for (size_t i = 0; i < size; i += 8) {
    state += 0x9E3779B97F4A7C15ull;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    for (size_t j = 0; j < 8 && i + j < size; j++)
      data[i + j] = static_cast<uint8_t>(z >> (8 * j));
  }
}
