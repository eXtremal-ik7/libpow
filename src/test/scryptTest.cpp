// scrypt(N=1024, r=1, p=1) over an 80 byte block header, the Litecoin proof of
// work. Two outside references: a table from scryptVectors.py (RFC 7914 checked
// against hashlib.scrypt) and the Litecoin genesis block, whose hash has to come
// out under the target of its own header
#include "scryptTest.h"
#include "hashTestUtils.h"
#include "scrypt.h"
#include <string.h>

namespace {

#include "scryptVectors.h"

constexpr uint64_t DataSeed = 0x5343525950540000ull;
constexpr size_t HeaderSize = 80;
constexpr size_t Count = sizeof(ScryptVectors) / sizeof(ScryptVectors[0]);
constexpr size_t DataSize = HeaderSize * Count + 16;

}

bool testScryptVectors()
{
  static uint8_t data[DataSize];
  fillTestData(data, sizeof(data), DataSeed);

  for (size_t i = 0; i < Count; i++) {
    // Off by i % 16, so most of the headers read are unaligned; the output is
    // put off its alignment too, every other time
    const uint8_t *header = data + HeaderSize * i + i % 16;
    uint8_t out[33];
    memset(out, 0, sizeof(out));
    uint8_t *hash = out + i % 2;
    scrypt_1024_1_1_256(header, hash);
    if (!checkHash(hash, 32, ScryptVectors[i], "scrypt", i))
      return false;
  }

  return true;
}

bool testScryptLitecoinGenesis()
{
  // Litecoin block 0: its sha256d is 12a765e3...bfe2, the block id everyone knows
  const char *headerHex =
    "01000000"
    "0000000000000000000000000000000000000000000000000000000000000000"
    "d9ced4ed1130f7b7faad9be25323ffafa33232a17c3edf6cfd97bee6bafbdd97"
    "b9aa8e4e"
    "f0ff0f1e"
    "cd513f7c";
  const char *expected = "001e67b013726fd7382e9acb69165b4b6316227fb3156b5b414ba6340c050000";

  uint8_t header[HeaderSize];
  fromHex(headerHex, header, sizeof(header));
  uint8_t hash[32];
  scrypt_1024_1_1_256(header, hash);
  if (!checkHash(hash, 32, expected, "litecoin genesis", sizeof(header)))
    return false;

  // The proof itself: the hash read as a little endian number is at most the
  // target that bits 0x1e0ffff0 encode, 0x0ffff0 << 8 * (0x1e - 3)
  uint8_t target[32];
  memset(target, 0, sizeof(target));
  target[0x1e - 1] = 0x0f;
  target[0x1e - 2] = 0xff;
  target[0x1e - 3] = 0xf0;
  for (size_t i = 32; i-- > 0; ) {
    if (hash[i] != target[i]) {
      if (hash[i] > target[i]) {
        const auto location = std::source_location::current();
        fprintf(stderr,
                " * %s:%u litecoin genesis hash is above its target\n",
                location.file_name(),
                static_cast<unsigned>(location.line()));
        return false;
      }
      break;
    }
  }

  return true;
}
