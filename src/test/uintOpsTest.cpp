// Bitwise, signed and modular operations of UInt - the rest is in uintTest.cpp.
// The tables in uintOpsVectors.h come from an outside source (python big ints),
// and the random part checks identities a table cannot reach
#include "uintOpsTest.h"
#include "uint.h"
#include <stdio.h>

namespace {

template<unsigned Bits>
bool cmphex(const UInt<Bits> &v, const char *ref, const char *what, const char *file, unsigned line) {
  if (v.getHex(false) != ref) {
    fprintf(stderr, " * %s:%u %s mismatch got %s should be %s\n", file, line, what, v.getHex(false).c_str(), ref);
    return false;
  }

  return true;
}

#define CHECK(value, expected, what) \
  do { if (!cmphex(value, expected, what, __FILE__, __LINE__)) return false; } while (0)

#define REQUIRE(condition, what) \
  do { \
    if (!(condition)) { \
      fprintf(stderr, " * %s:%u %s failed\n", __FILE__, __LINE__, what); \
      return false; \
    } \
  } while (0)

struct CBitwiseVector { const char *A, *B, *And, *Or, *Xor; };
struct CShiftVector { const char *A; unsigned Amount; const char *Result; };
struct CByteVector { const char *A; unsigned Index; unsigned Result; };
struct CDivVector { const char *A, *B, *Quotient, *Remainder; };
struct CTernaryVector { const char *A, *B, *C, *Result; };
struct CTernaryVector2 { const char *A, *B, *Result; };

#include "uintOpsVectors.h"

using U256 = UInt<256>;

// A deterministic stream: the test has to fail the same way on every machine
class CRandom {
public:
  explicit CRandom(uint64_t seed) : State_(seed) {}

  uint64_t next() {
    State_ += 0x9E3779B97F4A7C15ull;
    uint64_t z = State_;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }

private:
  uint64_t State_;
};

// Values of mixed shape: short, full width, holed with zeros and up against a
// limb boundary - the shape is what decides which division branches get hit
template<unsigned Bits>
UInt<Bits> randomValue(CRandom &rng) {
  constexpr unsigned Limbs = Bits / 64;
  UInt<Bits> value;
  const unsigned limbs = 1 + static_cast<unsigned>(rng.next() % Limbs);

  for (unsigned i = 0; i < limbs; i++) {
    uint64_t word = rng.next();
    switch (rng.next() % 8) {
      case 0 : word &= 0xFF; break;
      case 1 : word = 0; break;
      case 2 : word |= uint64_t{1} << 63; break;
      case 3 : word = ~uint64_t{0}; break;
      default : break;
    }
    value.data()[i] = word;
  }

  return value;
}

// Modular addition head on, for already reduced operands: a carry out of the
// width means the sum is above the modulus for certain
template<unsigned Bits>
UInt<Bits> refAddmodReduced(const UInt<Bits> &a, const UInt<Bits> &b, const UInt<Bits> &m) {
  UInt<Bits> sum = a + b;
  if (sum < a || !(sum < m))
    sum -= m;
  return sum;
}

template<unsigned Bits>
UInt<Bits> refAddmod(const UInt<Bits> &a, const UInt<Bits> &b, const UInt<Bits> &m) {
  if (m.isZero())
    return UInt<Bits>::zero();
  return refAddmodReduced(a % m, b % m, m);
}

// Modular multiply by doubling: no wide product and no Knuth division, so the
// check does not lean on anything mulmod itself uses
template<unsigned Bits>
UInt<Bits> refMulmod(const UInt<Bits> &a, const UInt<Bits> &b, const UInt<Bits> &m) {
  if (m.isZero())
    return UInt<Bits>::zero();

  const UInt<Bits> x = a % m;
  const UInt<Bits> y = b % m;

  UInt<Bits> result = UInt<Bits>::zero();
  for (unsigned i = y.bits(); i-- > 0; ) {
    result = refAddmodReduced(result, result, m);
    if ((y.data()[i / 64] >> (i % 64)) & 1)
      result = refAddmodReduced(result, x, m);
  }

  return result;
}

// u == q*v + r with the remainder below the divisor: a check on division that
// does not care which path inside divmod produced the digits
template<unsigned Bits>
bool divmodHolds(const UInt<Bits> &u, const UInt<Bits> &v) {
  UInt<Bits> q, r;
  u.divmod(v, &q, &r);

  UInt<2*Bits> back = UInt<Bits>::wideProduct(q, v);
  back += r;
  return back == UInt<2*Bits>(u) && r < v;
}

} // namespace

bool testUintBitwise()
{
  for (const CBitwiseVector &v : BitwiseVectors) {
    const U256 a = U256::fromHex(v.A);
    const U256 b = U256::fromHex(v.B);
    CHECK(a & b, v.And, "and");
    CHECK(a | b, v.Or, "or");
    CHECK(a ^ b, v.Xor, "xor");

    U256 x = a; x &= b;
    CHECK(x, v.And, "and=");
    x = a; x |= b;
    CHECK(x, v.Or, "or=");
    x = a; x ^= b;
    CHECK(x, v.Xor, "xor=");

    // ~a == -a - 1 in two's complement
    U256 inverted = ~a;
    inverted.add64(uint64_t{1});
    CHECK(inverted, (-a).getHex(false).c_str(), "not");
  }

  // A narrower operand reads as zero extended: AND clears the limbs above it,
  // OR and XOR leave them alone
  {
    const U256 wide = U256::fromHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    const UInt<128> narrow = UInt<128>::fromHex("00000000000000ff00000000000000ff");
    CHECK(wide & narrow, "ff00000000000000ff", "and narrow");
    CHECK(wide | narrow, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "or narrow");
    CHECK(wide ^ narrow, "ffffffffffffffffffffffffffffffff"
                         "ffffffffffffff00ffffffffffffff00", "xor narrow");
  }

  // The compound operators hand back a reference, so a chain updates the object
  {
    U256 x = U256::fromHex("f0f0");
    (x &= U256::fromHex("ff00")) |= U256::fromHex("000f");
    CHECK(x, "f00f", "chained assignment");
  }

  return true;
}

bool testUintSar()
{
  for (const CShiftVector &v : SarVectors) {
    U256 a = U256::fromHex(v.A);
    a.sar(v.Amount);
    CHECK(a, v.Result, "sar");

    // the same shift, given as a full width value
    a = U256::fromHex(v.A);
    a.sar(U256(static_cast<uint64_t>(v.Amount)));
    CHECK(a, v.Result, "sar(UInt)");
  }

  // A shift amount too wide for a limb also means "past the width"
  {
    U256 negative = U256::fromHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    U256 huge = U256::fromHex("100000000000000000000000000000000");
    negative.sar(huge);
    CHECK(negative, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "sar huge");

    U256 positive = U256::fromHex("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    positive.sar(huge);
    CHECK(positive, "0", "sar huge positive");

    U256 value = U256::fromHex("ff00");
    value.shl(huge);
    CHECK(value, "0", "shl huge");
    value = U256::fromHex("ff00");
    value.shr(huge);
    CHECK(value, "0", "shr huge");
  }

  return true;
}

bool testUintByte()
{
  for (const CByteVector &v : ByteVectors) {
    const U256 a = U256::fromHex(v.A);
    if (a.byte(v.Index) != v.Result) {
      fprintf(stderr, " * %s:%u byte mismatch for %s[%u]: got %u should be %u\n",
              __FILE__, __LINE__, v.A, v.Index, a.byte(v.Index), v.Result);
      return false;
    }
  }

  // Bytes in the order the word travels in outside the machine
  {
    const U256 a = U256::fromHex("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    uint8_t bytes[32];
    a.exportBE(bytes);
    for (unsigned i = 0; i < 32; i++)
      REQUIRE(bytes[i] == a.byte(i), "byte order");
  }

  return true;
}

bool testUintSignExtend()
{
  for (const CShiftVector &v : SignExtendVectors) {
    U256 a = U256::fromHex(v.A);
    a.signExtend(v.Amount);
    CHECK(a, v.Result, "signExtend");

    a = U256::fromHex(v.A);
    a.signExtend(U256(static_cast<uint64_t>(v.Amount)));
    CHECK(a, v.Result, "signExtend(UInt)");
  }

  // The byte index may not fit a limb either
  {
    U256 a = U256::fromHex("ff");
    a.signExtend(U256::fromHex("10000000000000000"));
    CHECK(a, "ff", "signExtend huge");
  }

  return true;
}

bool testUintSignedCmp()
{
  static const char *values[] = {
    "0", "1", "2", "ff", "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
    "8000000000000000000000000000000000000000000000000000000000000000",
    "8000000000000000000000000000000000000000000000000000000000000001",
    "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
    "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe"
  };

  // The order in the list is the signed order: the first five are nonnegative
  // and rising, the negative ones follow, rising as well
  static const int order[] = { 5, 6, 8, 7, 0, 1, 2, 3, 4 };

  for (unsigned i = 0; i < 9; i++) {
    for (unsigned j = 0; j < 9; j++) {
      const U256 a = U256::fromHex(values[order[i]]);
      const U256 b = U256::fromHex(values[order[j]]);

      REQUIRE(a.sless(b) == (i < j), "sless");
      REQUIRE((a.scmp(b) < 0) == (i < j), "scmp less");
      REQUIRE((a.scmp(b) > 0) == (i > j), "scmp greater");
      REQUIRE((a.scmp(b) == 0) == (i == j), "scmp equal");
      REQUIRE(a.isNegative() == (i < 4), "isNegative");
    }
  }

  // Top limbs equal - the ones below settle it
  {
    const U256 a = U256::fromHex("8000000000000000000000000000000100000000000000000000000000000000");
    const U256 b = U256::fromHex("8000000000000000000000000000000000000000000000000000000000000000");
    REQUIRE(b.sless(a) && !a.sless(b), "sless below the top limb");
    REQUIRE(a.scmp(b) > 0 && b.scmp(a) < 0, "scmp below the top limb");

    const U256 c = U256::fromHex("1000000000000000000000000000000000000000000000000000000000000001");
    const U256 d = U256::fromHex("1000000000000000000000000000000000000000000000000000000000000000");
    REQUIRE(d.sless(c) && !c.sless(d), "sless in the lowest limb");
    REQUIRE(c.scmp(d) > 0 && d.scmp(c) < 0, "scmp in the lowest limb");
    REQUIRE(c.scmp(c) == 0, "scmp of equal values");
  }

  return true;
}

bool testUintSdivmod()
{
  for (const CDivVector &v : SdivmodVectors) {
    const U256 a = U256::fromHex(v.A);
    const U256 b = U256::fromHex(v.B);

    U256 q, r;
    a.sdivmod(b, &q, &r);
    CHECK(q, v.Quotient, "sdiv");
    CHECK(r, v.Remainder, "smod");

    // the outputs are independent: each has to work on its own
    U256 only;
    a.sdivmod(b, &only, nullptr);
    CHECK(only, v.Quotient, "sdiv only");
    a.sdivmod(b, nullptr, &only);
    CHECK(only, v.Remainder, "smod only");
  }

  return true;
}

bool testUintAddmod()
{
  for (const CTernaryVector &v : AddmodVectors) {
    const U256 a = U256::fromHex(v.A);
    const U256 b = U256::fromHex(v.B);
    const U256 m = U256::fromHex(v.C);
    CHECK(U256::addmod(a, b, m), v.Result, "addmod");
    CHECK(U256::addmod(b, a, m), v.Result, "addmod reversed");
  }

  return true;
}

bool testUintMulmod()
{
  for (const CTernaryVector &v : MulmodVectors) {
    const U256 a = U256::fromHex(v.A);
    const U256 b = U256::fromHex(v.B);
    const U256 m = U256::fromHex(v.C);
    CHECK(U256::mulmod(a, b, m), v.Result, "mulmod");
    CHECK(U256::mulmod(b, a, m), v.Result, "mulmod reversed");
  }

  return true;
}

bool testUintExp()
{
  for (const CTernaryVector2 &v : ExpVectors) {
    const U256 base = U256::fromHex(v.A);
    const U256 e = U256::fromHex(v.B);
    CHECK(U256::exp(base, e), v.Result, "exp");
  }

  return true;
}

bool testUintWideProduct()
{
  for (const CTernaryVector2 &v : WideProductVectors) {
    const U256 a = U256::fromHex(v.A);
    const U256 b = U256::fromHex(v.B);
    CHECK(U256::wideProduct(a, b), v.Result, "wideProduct");
    CHECK(U256::wideProduct(b, a), v.Result, "wideProduct reversed");
  }

  // Operands of different widths: the result is as wide as both together
  {
    const U256 a = U256::fromHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    const UInt<128> b = UInt<128>::fromHex("ffffffffffffffffffffffffffffffff");
    CHECK(U256::wideProduct(a, b),
          "fffffffffffffffffffffffffffffffeffffffffffffffffffffffffffffffff"
          "00000000000000000000000000000001", "wideProduct mixed");
  }

  return true;
}

bool testUintBytes()
{
  static const char *values[] = {
    "0", "1", "ff", "100", "ffffffffffffffff", "10000000000000000",
    "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20",
    "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
  };

  for (const char *hex : values) {
    const U256 a = U256::fromHex(hex);

    uint8_t be[32], le[32];
    a.exportBE(be);
    a.exportLE(le);
    for (unsigned i = 0; i < 32; i++)
      REQUIRE(be[i] == le[31 - i], "exportLE mirrors exportBE");

    U256 loaded;
    loaded.importBE(be);
    REQUIRE(loaded == a, "importBE");

    REQUIRE(a.bytes() == (a.bits() + 7) / 8, "bytes");
  }

  REQUIRE(U256::zero().bytes() == 0, "bytes of zero");
  REQUIRE(U256::max().bytes() == 32, "bytes of max");
  REQUIRE(UInt<128>::max().bytes() == 16, "bytes of max 128");

  return true;
}

// Knuth corrects a quotient digit in two places: the short D3 loop and the rare
// D6 add back. Random numbers never reach D6, its odds are about 2/2^64, so the
// dividends here are built backwards from the answer
bool testUintDivmodCorrections()
{
  // A divisor with a tiny low limb: the estimate off the top two limbs knows
  // nothing about it and overshoots by one
  auto sparseDivisor = [](unsigned limbs) {
    U256 v = U256::zero();
    v.data()[0] = 1;
    v.data()[limbs - 1] = uint64_t{1} << 63;
    return v;
  };

  for (uint64_t middle : {uint64_t{0}, uint64_t{1}, uint64_t{1} << 63, ~uint64_t{0}}) {
    U256 v = sparseDivisor(3);
    v.data()[1] = middle;

    // u = (2^64 - 1)*v - 1, that is quotient 2^64 - 2 and remainder v - 1
    U256 u = v;
    u.mul64(~uint64_t{0});
    u.sub64(uint64_t{1});

    U256 q, r;
    u.divmod(v, &q, &r);
    REQUIRE(q == U256(~uint64_t{0} - 1), "add back quotient");
    REQUIRE(r == v - U256(uint64_t{1}), "add back remainder");
    REQUIRE(divmodHolds(u, v), "add back identity");
  }

  // Top limb of the dividend equal to the top limb of the divisor: the estimate
  // would be 2^64, and the saturated 2^64 - 1 is taken instead
  {
    U256 v = U256::zero();
    v.data()[0] = 1;
    v.data()[2] = uint64_t{1} << 63;

    U256 u = U256::zero();
    u.data()[0] = 2;
    u.data()[1] = uint64_t{1} << 63;
    u.data()[2] = uint64_t{1} << 63;
    REQUIRE(divmodHolds(u, v), "saturated estimate");

    // the same, but the estimate remainder overflows a limb and cancels D3
    u.data()[1] = ~uint64_t{0};
    REQUIRE(divmodHolds(u, v), "saturated estimate with overflow");
  }

  // The wide path, where the digits come off a reciprocal: it is only taken from
  // four digits up, which is why the dividends here are longer
  {
    const uint64_t high = uint64_t{1} << 63;
    const uint64_t middle = 0x123456789abcdef0ull;

    UInt<512> v = UInt<512>::zero();
    v.data()[0] = 2;
    v.data()[1] = middle;
    v.data()[2] = high;

    // The remainder after the first digit matches the divisor in both top limbs:
    // the next estimate then comes out 2^64 and has to be knocked down by hand.
    // The first digit also overshoots, so the divisor is added back
    UInt<512> u = UInt<512>::zero();
    u.data()[3] = 1;
    u.data()[4] = middle;
    u.data()[5] = high;
    u.data()[0] = 0xfedcba9876543210ull;
    REQUIRE(divmodHolds(u, v), "wide saturated estimate");

    // and a plain add back on the same path: a divisor with a tiny low limb and
    // a dividend built from the answer
    UInt<512> sparse = UInt<512>::zero();
    sparse.data()[0] = 1;
    sparse.data()[2] = high;

    UInt<512> w = sparse;
    w.mul64(~uint64_t{0});
    w.sub64(uint64_t{1});
    w.shl(3 * 64);
    REQUIRE(divmodHolds(w, sparse), "wide add back identity");
  }

  // Two corrections inside the three-by-two division turn on an exact equality of
  // 64-bit values. Such divisors never come up by chance and were solved for: the
  // compared quantity is monotone in one limb, so that limb is computed
  {
    static const struct { uint64_t D1, D0, U2, U1, U0; } cases[] = {
      // p == d1 while preparing the reciprocal, both sides of the inner condition
      { 0x8f078b9e474afed8ull, 0xe0e5dc4c72ff7099ull, 1, 2, 3 },
      { 0x823dbea5745bbaa2ull, 0x96c7ae0724bfa499ull, 1, 2, 3 },
      // r1 == d1 in the last correction of a digit, both sides again
      { 0x8f0b49b38c72fbabull, 0x321d92cc70fc99e7ull,
        0x080ec39a8e825a28ull, 0xe1cd2b9eaa568746ull, 0x4b95024c3e411dafull },
      { 0x863be659f52fd157ull, 0x4b08286e1ce4f549ull,
        0x50c122c3795cc062ull, 0xe0628f0da9b49a0full, 0xfeae953d3011025dull },
    };

    for (const auto &item : cases) {
      UInt<512> v = UInt<512>::zero();
      v.data()[0] = item.D0;
      v.data()[1] = item.D1;

      // A five limb dividend: the top pair goes through the first digit exactly and
      // lands in the next estimate in the shape the case needs
      UInt<512> u = UInt<512>::zero();
      u.data()[4] = item.U2;
      u.data()[3] = item.U1;
      u.data()[2] = item.U0;
      REQUIRE(divmodHolds(u, v), "three by two corrections");
    }
  }

  return true;
}

// The random part: identities where a table of references cannot help, and all
// three widths, because the division branch is picked by the limb count
bool testUintIdentities()
{
  CRandom rng(20260813);

  for (unsigned round = 0; round < 3000; round++) {
    const U256 a = randomValue<256>(rng);
    const U256 b = randomValue<256>(rng);
    const U256 m = randomValue<256>(rng);

    if (!b.isZero()) {
      REQUIRE(divmodHolds(a, b), "divmod identity");

      // signed division: quotient and remainder have to rebuild the dividend
      U256 q, r;
      a.sdivmod(b, &q, &r);
      REQUIRE(q * b + r == a, "sdivmod identity");
      REQUIRE(r.isZero() || r.isNegative() == a.isNegative(), "sdivmod remainder sign");
    }

    REQUIRE(U256::addmod(a, b, m) == refAddmod(a, b, m), "addmod");
    REQUIRE(U256::mulmod(a, b, m) == refMulmod(a, b, m), "mulmod");

    // exponentiation against repeated multiplication
    {
      const unsigned e = static_cast<unsigned>(rng.next() % 12);
      U256 expected = U256(uint64_t{1});
      for (unsigned i = 0; i < e; i++)
        expected *= a;
      REQUIRE(U256::exp(a, U256(static_cast<uint64_t>(e))) == expected, "exp");
    }

    // shifts: left and back right leaves whatever did not fall off the edge
    {
      const unsigned shift = static_cast<unsigned>(rng.next() % 300);
      U256 lowMask = U256::max();
      lowMask.shr(shift);        // ones where the left shift lost nothing

      U256 back = a;
      back.shl(shift);
      back.shr(shift);
      REQUIRE(back == (a & lowMask), "shl/shr roundtrip");

      // an arithmetic shift differs from a logical one only in the padding
      U256 arithmetic = a;
      arithmetic.sar(shift);
      U256 logical = a;
      logical.shr(shift);
      REQUIRE(arithmetic == (a.isNegative() ? (logical | ~lowMask) : logical), "sar");
    }

    // string conversions round trip
    REQUIRE(U256::fromHex(a.getHex(false).c_str()) == a, "hex roundtrip");
    REQUIRE(U256::fromHex(a.getHex(true, true, true).c_str()) == a, "hex roundtrip upper");

    // bytes round trip
    {
      uint8_t bytes[32];
      a.exportBE(bytes);
      U256 loaded;
      loaded.importBE(bytes);
      REQUIRE(loaded == a, "bytes roundtrip");
    }
  }

  // The same identities at other widths: which digit path runs depends on the
  // limb count
  for (unsigned round = 0; round < 1500; round++) {
    const UInt<128> a128 = randomValue<128>(rng);
    const UInt<128> b128 = randomValue<128>(rng);
    if (!b128.isZero())
      REQUIRE(divmodHolds(a128, b128), "divmod identity 128");

    const UInt<512> a512 = randomValue<512>(rng);
    const UInt<512> b512 = randomValue<512>(rng);
    const UInt<512> m512 = randomValue<512>(rng);
    if (!b512.isZero())
      REQUIRE(divmodHolds(a512, b512), "divmod identity 512");
    REQUIRE(UInt<512>::addmod(a512, b512, m512) == refAddmod(a512, b512, m512), "addmod 512");
    REQUIRE(UInt<512>::mulmod(a512, b512, m512) == refMulmod(a512, b512, m512), "mulmod 512");
  }

  return true;
}
