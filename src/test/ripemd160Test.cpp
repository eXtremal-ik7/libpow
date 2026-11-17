// RIPEMD-160 against two outside references: the vectors of the RIPEMD-160
// paper and a table over every length up to 1023 (ripemd160Vectors.py). The
// lengths walk each padding case - tail under 56 bytes, 56..63 with a second
// block of length only, and whole blocks - on unaligned input
#include "ripemd160Test.h"
#include "hashTestUtils.h"
#include "libpow/ripemd160.h"
#include <string.h>
#include <vector>

namespace {

#include "ripemd160Vectors.h"

constexpr uint64_t DataSeed = 0x524D443136300000ull;
constexpr size_t DataSize = 1280;
constexpr size_t MaxLength = sizeof(Ripemd160LengthVectors) / sizeof(Ripemd160LengthVectors[0]);
static_assert(MaxLength + 16 <= DataSize);

void hashStreaming(const uint8_t *data, size_t size, size_t chunk, uint8_t hash[20])
{
  CCtxRipemd160 ctx;
  ripemd160Init(&ctx);
  for (size_t offset = 0; offset < size; offset += chunk)
    ripemd160Update(&ctx, data + offset, chunk < size - offset ? chunk : size - offset);
  ripemd160Final(&ctx, hash);
}

}

bool testRipemd160Paper()
{
  struct CVector {
    const char *Message;
    const char *Hash;
  };

  static const CVector vectors[] = {
    {"", "9c1185a5c5e9fc54612808977ee8f548b2258d31"},
    {"a", "0bdc9d2d256b3ee9daae347be6f4dc835a467ffe"},
    {"abc", "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc"},
    {"message digest", "5d0689ef49d2fae572b881b123a85ffa21595f36"},
    {"abcdefghijklmnopqrstuvwxyz", "f71c27109c692c1b56bbdceb5b9d2865b3708dbc"},
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "12a053384a9c0c88e405a06c27dcf49ada62eb2b"},
    {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "b0e20b6e3116640286ed3a87a5713079b21f5189"},
    {"12345678901234567890123456789012345678901234567890123456789012345678901234567890", "9b752e45573d4b39f4dbd3323cab82bf63326bfb"}
  };

  for (const auto &v: vectors) {
    const size_t size = strlen(v.Message);
    uint8_t hash[20];
    ripemd160(v.Message, size, hash);
    if (!checkHash(hash, 20, v.Hash, "direct", size))
      return false;

    hashStreaming(reinterpret_cast<const uint8_t*>(v.Message), size, size ? size : 1, hash);
    if (!checkHash(hash, 20, v.Hash, "context", size))
      return false;
  }

  // One million of 'a': the length field needs more than 16 bits
  const char *million = "52783243c1697bdbe16d37f97f68f08325dc1528";
  const std::vector<uint8_t> a(1000000, 'a');
  uint8_t hash[20];
  ripemd160(a.data(), a.size(), hash);
  if (!checkHash(hash, 20, million, "direct", a.size()))
    return false;

  hashStreaming(a.data(), a.size(), 1000, hash);
  if (!checkHash(hash, 20, million, "context", a.size()))
    return false;

  return true;
}

bool testRipemd160Lengths()
{
  uint8_t data[DataSize];
  fillTestData(data, sizeof(data), DataSeed);

  for (size_t size = 0; size < MaxLength; size++) {
    // Off by size % 16, so most of the blocks read are unaligned
    const uint8_t *message = data + size % 16;
    const char *expected = Ripemd160LengthVectors[size];
    uint8_t hash[20];

    memset(hash, 0, sizeof(hash));
    ripemd160(message, size, hash);
    if (!checkHash(hash, 20, expected, "direct", size))
      return false;

    memset(hash, 0, sizeof(hash));
    hashStreaming(message, size, size ? size : 1, hash);
    if (!checkHash(hash, 20, expected, "context", size))
      return false;

    // In three parts: the second one starts inside a partly filled buffer
    CCtxRipemd160 ctx;
    ripemd160Init(&ctx);
    ripemd160Update(&ctx, message, size / 3);
    ripemd160Update(&ctx, message + size / 3, size / 3);
    ripemd160Update(&ctx, message + 2 * (size / 3), size - 2 * (size / 3));
    memset(hash, 0, sizeof(hash));
    ripemd160Final(&ctx, hash);
    if (!checkHash(hash, 20, expected, "context in three parts", size))
      return false;
  }

  return true;
}

bool testRipemd160Streaming()
{
  uint8_t data[DataSize];
  fillTestData(data, sizeof(data), DataSeed);

  // Chunk sizes on both sides of the block and of the padding boundary
  static const size_t chunks[] = {1, 3, 7, 55, 56, 57, 63, 64, 65, 127, 128, 129};
  for (size_t size = 0; size < MaxLength; size++) {
    for (size_t chunk: chunks) {
      uint8_t hash[20];
      memset(hash, 0, sizeof(hash));
      hashStreaming(data + size % 16, size, chunk, hash);
      if (!checkHash(hash, 20, Ripemd160LengthVectors[size], "streaming", size)) {
        fprintf(stderr, "   chunk=%zu\n", chunk);
        return false;
      }
    }
  }

  // An empty update anywhere changes nothing
  {
    CCtxRipemd160 ctx;
    ripemd160Init(&ctx);
    ripemd160Update(&ctx, data, 0);
    ripemd160Update(&ctx, data, 64);
    ripemd160Update(&ctx, data + 64, 0);
    ripemd160Update(&ctx, data + 64, 64);
    uint8_t hash[20];
    ripemd160Final(&ctx, hash);
    uint8_t reference[20];
    ripemd160(data, 128, reference);
    if (!checkHash(hash, 20, toHex(reference, 20).c_str(), "empty updates", 128))
      return false;
  }

  return true;
}

bool testRipemd160LowLevel()
{
  // "abc" padded by hand to a single block: the length is little endian here
  uint8_t block[64];
  memset(block, 0, sizeof(block));
  memcpy(block, "abc", 3);
  block[3] = 0x80;
  block[56] = 0x18;

  uint32_t state[5];
  ripemd160llInit(state);
  ripemd160llTransform(state, block);
  uint8_t hash[20];
  ripemd160llFinal(state, hash);
  if (!checkHash(hash, 20, "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc", "llTransform/llFinal", 3))
    return false;

  return true;
}
