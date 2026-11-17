// SHA-256 against two outside references: the FIPS 180-4 example messages and a
// table of hashlib digests over every length up to 1023 (sha256Vectors.py). The
// lengths walk each padding case - tail under 56 bytes, 56..63 with a second
// block of length only, and whole blocks - on unaligned input
#include "sha256Test.h"
#include "hashTestUtils.h"
#include "sha256.h"
#include <string.h>
#include <vector>

namespace {

#include "sha256Vectors.h"

constexpr uint64_t DataSeed = 0x5348413235360000ull;
constexpr size_t DataSize = 1280;
constexpr size_t MaxLength = sizeof(Sha256LengthVectors) / sizeof(Sha256LengthVectors[0]);
static_assert(MaxLength + 16 <= DataSize);

void hashStreaming(const uint8_t *data, size_t size, size_t chunk, uint8_t hash[32])
{
  CCtxSha256 ctx;
  sha256Init(&ctx);
  for (size_t offset = 0; offset < size; offset += chunk)
    sha256Update(&ctx, data + offset, chunk < size - offset ? chunk : size - offset);
  sha256Final(&ctx, hash);
}

}

bool testSha256Nist()
{
  struct CVector {
    const char *Message;
    const char *Hash;
  };

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
    if (!checkHash(hash, 32, v.Hash, "direct", size))
      return false;

    hashStreaming(reinterpret_cast<const uint8_t*>(v.Message), size, size ? size : 1, hash);
    if (!checkHash(hash, 32, v.Hash, "context", size))
      return false;
  }

  // One million of 'a': the length field needs more than 16 bits
  const char *million = "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0";
  const std::vector<uint8_t> a(1000000, 'a');
  uint8_t hash[32];
  sha256(a.data(), a.size(), hash);
  if (!checkHash(hash, 32, million, "direct", a.size()))
    return false;

  hashStreaming(a.data(), a.size(), 1000, hash);
  if (!checkHash(hash, 32, million, "context", a.size()))
    return false;

  return true;
}

bool testSha256Lengths()
{
  uint8_t data[DataSize];
  fillTestData(data, sizeof(data), DataSeed);

  for (size_t size = 0; size < MaxLength; size++) {
    // Off by size % 16, so most of the blocks read are unaligned
    const uint8_t *message = data + size % 16;
    const char *expected = Sha256LengthVectors[size];
    uint8_t hash[32];

    memset(hash, 0, sizeof(hash));
    sha256(message, size, hash);
    if (!checkHash(hash, 32, expected, "direct", size))
      return false;

    memset(hash, 0, sizeof(hash));
    hashStreaming(message, size, size ? size : 1, hash);
    if (!checkHash(hash, 32, expected, "context", size))
      return false;

    // In three parts: the second one starts inside a partly filled buffer
    CCtxSha256 ctx;
    sha256Init(&ctx);
    sha256Update(&ctx, message, size / 3);
    sha256Update(&ctx, message + size / 3, size / 3);
    sha256Update(&ctx, message + 2 * (size / 3), size - 2 * (size / 3));
    memset(hash, 0, sizeof(hash));
    sha256Final(&ctx, hash);
    if (!checkHash(hash, 32, expected, "context in three parts", size))
      return false;
  }

  return true;
}

bool testSha256Streaming()
{
  uint8_t data[DataSize];
  fillTestData(data, sizeof(data), DataSeed);

  // Chunk sizes on both sides of the block and of the padding boundary
  static const size_t chunks[] = {1, 3, 7, 55, 56, 57, 63, 64, 65, 127, 128, 129};
  for (size_t size = 0; size < MaxLength; size++) {
    for (size_t chunk: chunks) {
      uint8_t hash[32];
      memset(hash, 0, sizeof(hash));
      hashStreaming(data + size % 16, size, chunk, hash);
      if (!checkHash(hash, 32, Sha256LengthVectors[size], "streaming", size)) {
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
    if (!checkHash(hash, 32, toHex(reference, 32).c_str(), "empty updates", 128))
      return false;
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
      const auto location = std::source_location::current();
      fprintf(stderr,
              " * %s:%u llTransform/llFinal without bswap mismatch\n",
              location.file_name(),
              static_cast<unsigned>(location.line()));
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
    if (!checkHash(hash, 32, expected, "llTransform/llFinal with bswap", 3))
      return false;
  }

  return true;
}
