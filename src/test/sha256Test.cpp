// SHA-256 against two outside references: the FIPS 180-4 example messages and a
// table of hashlib digests over every length up to 1023 (sha256Vectors.py). The
// lengths walk each padding case - tail under 56 bytes, 56..63 with a second
// block of length only, and whole blocks - on unaligned input
#include "sha256Test.h"
#include "sha256.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

namespace {

#include "sha256Vectors.h"

std::string toHex(const uint8_t *data, size_t size) {
  static const char digits[] = "0123456789abcdef";
  std::string result;
  for (size_t i = 0; i < size; i++) {
    result.push_back(digits[data[i] >> 4]);
    result.push_back(digits[data[i] & 0x0F]);
  }
  return result;
}

bool cmphash(const uint8_t hash[32], const char *ref, const char *what, size_t size, const char *file, unsigned line) {
  const std::string got = toHex(hash, 32);
  if (got != ref) {
    fprintf(stderr, " * %s:%u %s size=%zu mismatch got %s should be %s\n", file, line, what, size, got.c_str(), ref);
    return false;
  }

  return true;
}

#define CHECK(hash, expected, what, size) \
  do { if (!cmphash(hash, expected, what, size, __FILE__, __LINE__)) return false; } while (0)

// Must match sha256Vectors.py: splitmix64 from a fixed seed, little endian bytes
constexpr size_t DataSize = 1280;
constexpr size_t MaxLength = sizeof(Sha256LengthVectors) / sizeof(Sha256LengthVectors[0]);
static_assert(MaxLength + 16 <= DataSize);

void fillTestData(uint8_t *data) {
  uint64_t state = 0x5348413235360000ull;
  for (size_t i = 0; i < DataSize; i += 8) {
    state += 0x9E3779B97F4A7C15ull;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= z >> 31;
    for (size_t j = 0; j < 8; j++)
      data[i + j] = static_cast<uint8_t>(z >> (8 * j));
  }
}

void hashStreaming(const uint8_t *data, size_t size, size_t chunk, uint8_t hash[32]) {
  CCtxSha256 ctx;
  sha256Init(&ctx);
  for (size_t offset = 0; offset < size; offset += chunk)
    sha256Update(&ctx, data + offset, chunk < size - offset ? chunk : size - offset);
  sha256Final(&ctx, hash);
}

}

bool testSha256Nist()
{
  struct CVector { const char *Message; const char *Hash; };
  static const CVector vectors[] = {
    {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
    {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
    {"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"}
  };

  for (const auto &v: vectors) {
    const size_t size = strlen(v.Message);
    uint8_t hash[32];
    sha256(v.Message, size, hash);
    CHECK(hash, v.Hash, "direct", size);
    hashStreaming(reinterpret_cast<const uint8_t*>(v.Message), size, size ? size : 1, hash);
    CHECK(hash, v.Hash, "context", size);
  }

  // One million of 'a': the length field needs more than 16 bits
  const char *million = "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0";
  std::vector<uint8_t> a(1000000, 'a');
  uint8_t hash[32];
  sha256(a.data(), a.size(), hash);
  CHECK(hash, million, "direct", a.size());
  hashStreaming(a.data(), a.size(), 1000, hash);
  CHECK(hash, million, "context", a.size());
  return true;
}

bool testSha256Lengths()
{
  uint8_t data[DataSize];
  fillTestData(data);

  for (size_t size = 0; size < MaxLength; size++) {
    // Off by size % 16, so most of the blocks read are unaligned
    const uint8_t *message = data + size % 16;
    const char *expected = Sha256LengthVectors[size];
    uint8_t hash[32];

    memset(hash, 0, sizeof(hash));
    sha256(message, size, hash);
    CHECK(hash, expected, "direct", size);

    memset(hash, 0, sizeof(hash));
    hashStreaming(message, size, size ? size : 1, hash);
    CHECK(hash, expected, "context", size);

    // In three parts: the second one starts inside a partly filled buffer
    {
      CCtxSha256 ctx;
      sha256Init(&ctx);
      sha256Update(&ctx, message, size / 3);
      sha256Update(&ctx, message + size / 3, size / 3);
      sha256Update(&ctx, message + 2 * (size / 3), size - 2 * (size / 3));
      memset(hash, 0, sizeof(hash));
      sha256Final(&ctx, hash);
      CHECK(hash, expected, "context in three parts", size);
    }
  }

  return true;
}

bool testSha256Streaming()
{
  uint8_t data[DataSize];
  fillTestData(data);

  // Chunk sizes on both sides of the block and of the padding boundary
  static const size_t chunks[] = {1, 3, 7, 55, 56, 57, 63, 64, 65, 127, 128, 129};
  for (size_t size = 0; size < MaxLength; size++) {
    for (size_t chunk: chunks) {
      uint8_t hash[32];
      memset(hash, 0, sizeof(hash));
      hashStreaming(data + size % 16, size, chunk, hash);
      if (!cmphash(hash, Sha256LengthVectors[size], "streaming", size, __FILE__, __LINE__)) {
        fprintf(stderr, "   chunk=%zu\n", chunk);
        return false;
      }
    }
  }

  // An empty update anywhere changes nothing
  {
    CCtxSha256 ctx;
    sha256Init(&ctx);
    sha256Update(&ctx, data, 0);
    sha256Update(&ctx, data, 64);
    sha256Update(&ctx, data + 64, 0);
    sha256Update(&ctx, data + 64, 64);
    uint8_t hash[32];
    sha256Final(&ctx, hash);
    uint8_t reference[32];
    sha256(data, 128, reference);
    CHECK(hash, toHex(reference, 32).c_str(), "empty updates", size_t{128});
  }

  return true;
}

bool testSha256LowLevel()
{
  // "abc" padded by hand to a single block
  static const uint32_t words[16] = {0x61626380, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x18};
  static const uint32_t expectedState[8] = {
    0xba7816bf, 0x8f01cfea, 0x414140de, 0x5dae2223, 0xb00361a3, 0x96177a9c, 0xb410ff61, 0xf20015ad
  };
  const char *expected = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";

  // bswap = 0: the words are taken and the state is given as numbers
  {
    uint32_t state[8];
    sha256llInit(state);
    sha256llTransform(state, words, 0);
    uint8_t out[32];
    sha256llFinal(state, out, 0);
    uint32_t outWords[8];
    memcpy(outWords, out, sizeof(outWords));
    if (memcmp(outWords, expectedState, sizeof(expectedState)) != 0) {
      fprintf(stderr, " * %s:%u llTransform/llFinal without bswap mismatch\n", __FILE__, __LINE__);
      return false;
    }
  }

  // bswap = 1: the block is the message bytes, the hash comes out as bytes
  {
    uint8_t block[64];
    memset(block, 0, sizeof(block));
    memcpy(block, "abc", 3);
    block[3] = 0x80;
    block[63] = 0x18;
    uint32_t in[16];
    memcpy(in, block, sizeof(in));

    uint32_t state[8];
    sha256llInit(state);
    sha256llTransform(state, in, 1);
    uint8_t hash[32];
    sha256llFinal(state, hash, 1);
    CHECK(hash, expected, "llTransform/llFinal with bswap", size_t{3});
  }

  return true;
}
