// SHA-3 and Keccak against outside references: the FIPS 202 example messages,
// the published Keccak digests and tables from sha3Vectors.py (hashlib for SHA-3,
// a model checked against it for Keccak). The lengths run past several blocks of
// both rates in use, 136 bytes for the 256 bit hash and 72 for the 512 bit one,
// on unaligned input
#include "sha3Test.h"
#include "hashTestUtils.h"
#include "libpow/sha3.h"
#include <string.h>
#include <vector>

namespace {

struct CLengthVector {
  const char *Sha3Hash256;
  const char *Sha3Hash512;
  const char *KeccakHash256;
  const char *KeccakHash512;
};

#include "sha3Vectors.h"

constexpr uint64_t DataSeed = 0x5348413300000000ull;
constexpr size_t DataSize = 320;
constexpr size_t MaxLength = sizeof(Sha3LengthVectors) / sizeof(Sha3LengthVectors[0]);
static_assert(MaxLength + 16 <= DataSize);

// SHA-3 and Keccak differ in the padding byte only, so every check runs over both
// families through the same table
struct CVariant {
  const char *Name;
  unsigned HashSize;
  void (*Init)(CCtxSha3 *ctx, unsigned hashSize);
  void (*Update)(CCtxSha3 *ctx, const void *data, size_t size);
  void (*Final)(CCtxSha3 *ctx, uint8_t *hash);
  void (*Direct)(const void *data, size_t size, uint8_t *hash, unsigned hashSize);
  const char *CLengthVector::*Expected;
};

const CVariant Variants[] = {
  {"sha3-256", 32, sha3Init, sha3Update, sha3Final, sha3, &CLengthVector::Sha3Hash256},
  {"sha3-512", 64, sha3Init, sha3Update, sha3Final, sha3, &CLengthVector::Sha3Hash512},
  {"keccak-256", 32, keccakInit, keccakUpdate, keccakFinal, keccak, &CLengthVector::KeccakHash256},
  {"keccak-512", 64, keccakInit, keccakUpdate, keccakFinal, keccak, &CLengthVector::KeccakHash512}
};

void hashStreaming(const CVariant &variant, const uint8_t *data, size_t size, size_t chunk, uint8_t *hash)
{
  CCtxSha3 ctx;
  variant.Init(&ctx, variant.HashSize);
  for (size_t offset = 0; offset < size; offset += chunk)
    variant.Update(&ctx, data + offset, chunk < size - offset ? chunk : size - offset);
  variant.Final(&ctx, hash);
}

}

bool testSha3Nist()
{
  struct CVector {
    const char *Message;
    unsigned HashSize;
    const char *Hash;
  };

  static const char *const abc448 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
  static const char *const abc896 =
    "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

  // Every hash size the standard names: each one has a rate of its own
  static const CVector vectors[] = {
    {"", 28, "6b4e03423667dbb73b6e15454f0eb1abd4597f9a1b078e3f5b5a6bc7"},
    {"abc", 28, "e642824c3f8cf24ad09234ee7d3c766fc9a3a5168d0c94ad73b46fdf"},
    {abc448, 28, "8a24108b154ada21c9fd5574494479ba5c7e7ab76ef264ead0fcce33"},
    {abc896, 28, "543e6868e1666c1a643630df77367ae5a62a85070a51c14cbf665cbc"},
    {"", 32, "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a"},
    {"abc", 32, "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532"},
    {abc448, 32, "41c0dba2a9d6240849100376a8235e2c82e1b9998a999e21db32dd97496d3376"},
    {abc896, 32, "916f6061fe879741ca6469b43971dfdb28b1a32dc36cb3254e812be27aad1d18"},
    {"", 48, "0c63a75b845e4f7d01107d852e4c2485c51a50aaaa94fc61995e71bbee983a2ac3713831264adb47fb6bd1e058d5f004"},
    {"abc", 48, "ec01498288516fc926459f58e2c6ad8df9b473cb0fc08c2596da7cf0e49be4b298d88cea927ac7f539f1edf228376d25"},
    {abc448, 48, "991c665755eb3a4b6bbdfb75c78a492e8c56a22c5c4d7e429bfdbc32b9d4ad5aa04a1f076e62fea19eef51acd0657c22"},
    {abc896, 48, "79407d3b5916b59c3e30b09822974791c313fb9ecc849e406f23592d04f625dc8c709b98b43b3852b337216179aa7fc7"},
    {"", 64, "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a6"
             "15b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26"},
    {"abc", 64, "b751850b1a57168a5693cd924b6b096e08f621827444f70d884f5d0240d2712e"
                "10e116e9192af3c91a7ec57647e3934057340b4cf408d5a56592f8274eec53f0"},
    {abc448, 64, "04a371e84ecfb5b8b77cb48610fca8182dd457ce6f326a0fd3d7ec2f1e91636d"
                 "ee691fbe0c985302ba1b0d8dc78c086346b533b49c030d99a27daf1139d6e75e"},
    {abc896, 64, "afebb2ef542e6579c50cad06d2e578f9f8dd6881d7dc824d26360feebf18a4fa"
                 "73e3261122948efcfd492e74e82e2189ed0fb440d187f382270cb455f21dd185"}
  };

  for (const auto &v: vectors) {
    const size_t size = strlen(v.Message);
    uint8_t hash[64];
    sha3(v.Message, size, hash, v.HashSize);
    if (!checkHash(hash, v.HashSize, v.Hash, "direct", size))
      return false;

    CCtxSha3 ctx;
    sha3Init(&ctx, v.HashSize);
    sha3Update(&ctx, v.Message, size);
    sha3Final(&ctx, hash);
    if (!checkHash(hash, v.HashSize, v.Hash, "context", size))
      return false;
  }

  // One million of 'a', in pieces that are no multiple of any rate
  static const CVector million[] = {
    {nullptr, 32, "5c8875ae474a3634ba4fd55ec85bffd661f32aca75c6d699d0cdcb6c115891c1"},
    {nullptr, 64, "3c3a876da14034ab60627c077bb98f7e120a2a5370212dffb3385a18d4f38859"
                  "ed311d0a9d5141ce9cc5c66ee689b266a8aa18ace8282a0e0db596c90b0a7b87"}
  };

  const std::vector<uint8_t> a(1000000, 'a');
  for (const auto &v: million) {
    uint8_t hash[64];
    sha3(a.data(), a.size(), hash, v.HashSize);
    if (!checkHash(hash, v.HashSize, v.Hash, "direct", a.size()))
      return false;

    CCtxSha3 ctx;
    sha3Init(&ctx, v.HashSize);
    for (size_t offset = 0; offset < a.size(); offset += 1000)
      sha3Update(&ctx, a.data() + offset, 1000);
    sha3Final(&ctx, hash);
    if (!checkHash(hash, v.HashSize, v.Hash, "context", a.size()))
      return false;
  }

  return true;
}

bool testSha3Keccak()
{
  // The published Keccak digests: the ones Ethereum and its tools are checked with
  struct CVector {
    const char *Message;
    unsigned HashSize;
    const char *Hash;
  };

  static const CVector vectors[] = {
    {"", 32, "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"},
    {"abc", 32, "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45"},
    {"", 64, "0eab42de4c3ceb9235fc91acffe746b29c29a8c366b7c60e4e67c466f36a4304"
             "c00fa9caf9d87976ba469bcbe06713b435f091ef2769fb160cdab33d3670680e"}
  };

  for (const auto &v: vectors) {
    const size_t size = strlen(v.Message);
    uint8_t hash[64];
    keccak(v.Message, size, hash, v.HashSize);
    if (!checkHash(hash, v.HashSize, v.Hash, "direct", size))
      return false;

    CCtxSha3 ctx;
    keccakInit(&ctx, v.HashSize);
    keccakUpdate(&ctx, v.Message, size);
    keccakFinal(&ctx, hash);
    if (!checkHash(hash, v.HashSize, v.Hash, "context", size))
      return false;
  }

  return true;
}

bool testSha3Lengths()
{
  uint8_t data[DataSize];
  fillTestData(data, sizeof(data), DataSeed);

  for (const auto &variant: Variants) {
    for (size_t size = 0; size < MaxLength; size++) {
      // Off by size % 16, so most of the lanes read are unaligned
      const uint8_t *message = data + size % 16;
      const char *expected = Sha3LengthVectors[size].*variant.Expected;
      uint8_t hash[64];

      memset(hash, 0, sizeof(hash));
      variant.Direct(message, size, hash, variant.HashSize);
      if (!checkHash(hash, variant.HashSize, expected, variant.Name, size))
        return false;

      memset(hash, 0, sizeof(hash));
      hashStreaming(variant, message, size, size ? size : 1, hash);
      if (!checkHash(hash, variant.HashSize, expected, variant.Name, size))
        return false;

      // In three parts: the second one starts inside a partly filled block,
      // mostly off a lane boundary
      CCtxSha3 ctx;
      variant.Init(&ctx, variant.HashSize);
      variant.Update(&ctx, message, size / 3);
      variant.Update(&ctx, message + size / 3, size / 3);
      variant.Update(&ctx, message + 2 * (size / 3), size - 2 * (size / 3));
      memset(hash, 0, sizeof(hash));
      variant.Final(&ctx, hash);
      if (!checkHash(hash, variant.HashSize, expected, variant.Name, size))
        return false;
    }
  }

  return true;
}

bool testSha3Streaming()
{
  uint8_t data[DataSize];
  fillTestData(data, sizeof(data), DataSeed);

  // Chunk sizes on both sides of a lane and of both rates
  static const size_t chunks[] = {1, 3, 7, 8, 9, 71, 72, 73, 135, 136, 137};
  for (const auto &variant: Variants) {
    for (size_t size = 0; size < MaxLength; size++) {
      for (size_t chunk: chunks) {
        uint8_t hash[64];
        memset(hash, 0, sizeof(hash));
        hashStreaming(variant, data + size % 16, size, chunk, hash);
        if (!checkHash(hash, variant.HashSize, Sha3LengthVectors[size].*variant.Expected, variant.Name, size)) {
          fprintf(stderr, "   chunk=%zu\n", chunk);
          return false;
        }
      }
    }
  }

  // An empty update anywhere changes nothing
  {
    CCtxSha3 ctx;
    sha3Init(&ctx, 32);
    sha3Update(&ctx, data, 0);
    sha3Update(&ctx, data, 100);
    sha3Update(&ctx, data + 100, 0);
    sha3Update(&ctx, data + 100, 100);
    uint8_t hash[32];
    sha3Final(&ctx, hash);
    uint8_t reference[32];
    sha3(data, 200, reference, 32);
    if (!checkHash(hash, 32, toHex(reference, 32).c_str(), "empty updates", 200))
      return false;
  }

  return true;
}

bool testSha3LowLevel()
{
  // Keccak-f[1600] on the all zero state
  {
    uint64_t state[25];
    memset(state, 0, sizeof(state));
    sha3llTransform(state);
    if (memcmp(state, Keccak1600OfZero, sizeof(state)) != 0) {
      const auto location = std::source_location::current();
      fprintf(stderr,
              " * %s:%u sha3llTransform of the zero state mismatch\n",
              location.file_name(),
              static_cast<unsigned>(location.line()));
      return false;
    }
  }

  // Keccak-p[800] as ProgPoW runs it: the last 12 of the 22 rounds, one call each
  {
    uint8_t data[DataSize];
    fillTestData(data, sizeof(data), DataSeed);
    uint32_t state[25];
    for (unsigned i = 0; i < 25; i++) {
      state[i] = static_cast<uint32_t>(data[4 * i]) |
                 static_cast<uint32_t>(data[4 * i + 1]) << 8 |
                 static_cast<uint32_t>(data[4 * i + 2]) << 16 |
                 static_cast<uint32_t>(data[4 * i + 3]) << 24;
    }

    for (unsigned round = 22 - 12; round < 22; round++)
      sha3llRound800(state, round);

    for (unsigned i = 0; i < 25; i++) {
      if (state[i] != Keccak800Of12Rounds[i]) {
        const auto location = std::source_location::current();
        fprintf(stderr,
                " * %s:%u sha3llRound800 lane %u mismatch got %08x should be %08x\n",
                location.file_name(),
                static_cast<unsigned>(location.line()),
                i,
                static_cast<unsigned>(state[i]),
                static_cast<unsigned>(Keccak800Of12Rounds[i]));
        return false;
      }
    }
  }

  return true;
}
