#include "uintTest.h"
#include "libpow/uint.h"

template<unsigned Bits>
static inline bool cmphex(const UInt<Bits> &v, const std::string &ref, const char *file, unsigned line) {
  if (v.getHex(false) != ref) {
    fprintf(stderr, " * %s:%u mismatch got %s should be %s\n", file, line, v.getHex(false).c_str(), ref.c_str());
    return false;
  }

  return true;
}

// Test for shift left (shl) function
bool testUintShl()
{
  bool acc = true;

  auto check = [&acc](const char *value, unsigned shift,
                      const char *expected, const char *file, unsigned line) {
    if (acc) {
      UInt<256> v = UInt<256>::fromHex(value);
      v <<= shift;
      acc = cmphex(v, expected, file, line);
      if (!acc) {
        fprintf(stderr, "   shl mismatch for %s << %u\n", value, shift);
      }
    }
  };

  // Shift by 0
  check("1", 0, "1", __FILE__, __LINE__);
  check("ffffffffffffffff", 0, "ffffffffffffffff", __FILE__, __LINE__);

  // Small shifts (< 64 bits)
  check("1", 1, "2", __FILE__, __LINE__);
  check("1", 4, "10", __FILE__, __LINE__);
  check("1", 8, "100", __FILE__, __LINE__);
  check("ff", 4, "ff0", __FILE__, __LINE__);
  check("ffffffffffffffff", 1, "1fffffffffffffffe", __FILE__, __LINE__);
  check("ffffffffffffffff", 4, "ffffffffffffffff0", __FILE__, __LINE__);

  // Shift by exactly 64 bits (one full limb)
  check("1", 64, "10000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffff", 64, "ffffffffffffffff0000000000000000", __FILE__, __LINE__);

  // Shift by 64 + small
  check("1", 65, "20000000000000000", __FILE__, __LINE__);
  check("1", 68, "100000000000000000", __FILE__, __LINE__);
  check("ff", 64, "ff0000000000000000", __FILE__, __LINE__);

  // Shift by 128 bits (two full limbs)
  check("1", 128, "100000000000000000000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffff", 128, "ffffffffffffffff00000000000000000000000000000000", __FILE__, __LINE__);

  // Shift by 192 bits (three full limbs)
  check("1", 192, "1000000000000000000000000000000000000000000000000", __FILE__, __LINE__);

  // Shift causing overflow (bits shifted out)
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 4,
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 64,
        "ffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000", __FILE__, __LINE__);

  // Shift >= Bits should result in 0
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 256, "0", __FILE__, __LINE__);
  check("1", 300, "0", __FILE__, __LINE__);

  // Edge cases with multi-limb values
  check("123456789abcdef0", 32, "123456789abcdef000000000", __FILE__, __LINE__);
  check("deadbeefcafebabe", 63, "6f56df77e57f5d5f0000000000000000", __FILE__, __LINE__);


  // A literal and a variable take different paths: the literal length is known
  // at compile time and the loop unrolls into a chain of two limb shifts
  {
    const UInt<256> value = UInt<256>::fromHex("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    volatile unsigned runtime = 0;

#define CHECK_CONSTANT_SHL(constant) \
    if (acc) { \
      UInt<256> byLiteral = value; \
      byLiteral.shl(constant); \
      runtime = (constant); \
      UInt<256> byVariable = value; \
      byVariable.shl(runtime); \
      if (!(byLiteral == byVariable)) { \
        fprintf(stderr, " * %s:%u constant shl %u differs from the variable one\n", __FILE__, __LINE__, unsigned(constant)); \
        acc = false; \
      } \
    }

    CHECK_CONSTANT_SHL(0)
    CHECK_CONSTANT_SHL(1)
    CHECK_CONSTANT_SHL(37)
    CHECK_CONSTANT_SHL(63)
    CHECK_CONSTANT_SHL(64)
    CHECK_CONSTANT_SHL(65)
    CHECK_CONSTANT_SHL(128)
    CHECK_CONSTANT_SHL(191)
    CHECK_CONSTANT_SHL(255)
    CHECK_CONSTANT_SHL(256)
    CHECK_CONSTANT_SHL(1000)
#undef CHECK_CONSTANT_SHL
  }

  return acc;
}
// Test for shift right (shr) function
bool testUintShr()
{
  bool acc = true;

  auto check = [&acc](const char *value, unsigned shift,
                      const char *expected, const char *file, unsigned line) {
    if (acc) {
      UInt<256> v = UInt<256>::fromHex(value);
      v >>= shift;
      acc = cmphex(v, expected, file, line);
      if (!acc) {
        fprintf(stderr, "   shr mismatch for %s >> %u\n", value, shift);
      }
    }
  };

  // Shift by 0
  check("1", 0, "1", __FILE__, __LINE__);
  check("ffffffffffffffff", 0, "ffffffffffffffff", __FILE__, __LINE__);

  // Small shifts (< 64 bits)
  check("2", 1, "1", __FILE__, __LINE__);
  check("10", 4, "1", __FILE__, __LINE__);
  check("100", 8, "1", __FILE__, __LINE__);
  check("ff0", 4, "ff", __FILE__, __LINE__);
  check("1fffffffffffffffe", 1, "ffffffffffffffff", __FILE__, __LINE__);
  check("ffffffffffffffff0", 4, "ffffffffffffffff", __FILE__, __LINE__);

  // Shift by exactly 64 bits (one full limb)
  check("10000000000000000", 64, "1", __FILE__, __LINE__);
  check("ffffffffffffffff0000000000000000", 64, "ffffffffffffffff", __FILE__, __LINE__);

  // Shift by 64 + small
  check("20000000000000000", 65, "1", __FILE__, __LINE__);
  check("100000000000000000", 68, "1", __FILE__, __LINE__);
  check("ff0000000000000000", 64, "ff", __FILE__, __LINE__);

  // Shift by 128 bits (two full limbs)
  check("100000000000000000000000000000000", 128, "1", __FILE__, __LINE__);
  check("ffffffffffffffff00000000000000000000000000000000", 128, "ffffffffffffffff", __FILE__, __LINE__);

  // Shift by 192 bits (three full limbs)
  check("1000000000000000000000000000000000000000000000000", 192, "1", __FILE__, __LINE__);

  // Shift causing underflow (bits shifted out)
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 4,
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 64,
        "ffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);

  // Shift >= Bits should result in 0
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 256, "0", __FILE__, __LINE__);
  check("1", 300, "0", __FILE__, __LINE__);

  // Edge cases with multi-limb values
  check("123456789abcdef000000000", 32, "123456789abcdef0", __FILE__, __LINE__);
  check("6f56df77e57f5d5f0000000000000000", 63, "deadbeefcafebabe", __FILE__, __LINE__);

  // Verify shl and shr are inverses
  {
    UInt<256> original = UInt<256>::fromHex("deadbeefcafebabedeadbeefcafebabe");
    UInt<256> v = original;
    v <<= 37;
    v >>= 37;
    if (!cmphex(v, original.getHex(false), __FILE__, __LINE__)) {
      fprintf(stderr, "   shl/shr inverse test failed\n");
      acc = false;
    }
  }


  // A literal and a variable take different paths: the literal length is known
  // at compile time and the loop unrolls into a chain of two limb shifts
  {
    const UInt<256> value = UInt<256>::fromHex("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    volatile unsigned runtime = 0;

#define CHECK_CONSTANT_SHR(constant) \
    if (acc) { \
      UInt<256> byLiteral = value; \
      byLiteral.shr(constant); \
      runtime = (constant); \
      UInt<256> byVariable = value; \
      byVariable.shr(runtime); \
      if (!(byLiteral == byVariable)) { \
        fprintf(stderr, " * %s:%u constant shr %u differs from the variable one\n", __FILE__, __LINE__, unsigned(constant)); \
        acc = false; \
      } \
    }

    CHECK_CONSTANT_SHR(0)
    CHECK_CONSTANT_SHR(1)
    CHECK_CONSTANT_SHR(37)
    CHECK_CONSTANT_SHR(63)
    CHECK_CONSTANT_SHR(64)
    CHECK_CONSTANT_SHR(65)
    CHECK_CONSTANT_SHR(128)
    CHECK_CONSTANT_SHR(191)
    CHECK_CONSTANT_SHR(255)
    CHECK_CONSTANT_SHR(256)
    CHECK_CONSTANT_SHR(1000)
#undef CHECK_CONSTANT_SHR
  }

  return acc;
}
bool testUintMul64()
{

  // 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff * 1
  // 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff * 2
  // 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff * 0xffffffffffffffff

  bool acc = true;
  auto check = [&acc](const char *l, uint64_t r, const char *ref, const char *file, unsigned line) {
    if (acc) {
      UInt<256> l256 = UInt<256>::fromHex(l);
      l256 *= r;
      acc = cmphex(l256, ref, file, line);
    }
  };

  check("0", 0ull, "0", __FILE__, __LINE__);
  check("0", 1ull, "0", __FILE__, __LINE__);
  check("1", 0ull, "0", __FILE__, __LINE__);
  check("ffffffffffffffff", 0ull, "0", __FILE__, __LINE__);
  check("ffffffffffffffff", 1ull, "ffffffffffffffff", __FILE__, __LINE__);
  check("ffffffffffffffff", 2ull, "1fffffffffffffffe", __FILE__, __LINE__);
  check("ffffffffffffffff", 0xffffffffffffffffull, "fffffffffffffffe0000000000000001", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0, "0", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 1, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 2, "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0xffffffffffffffffull, "ffffffffffffffffffffffffffffffffffffffffffffffff0000000000000001", __FILE__, __LINE__);
  check("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0xdeadbeefdeadbeef, "eeffffffffffffffffffffffffffffffffffffffffffffff2152411021524111", __FILE__, __LINE__);

  return acc;
}

bool testUintMul()
{
  bool acc = true;
  auto check = [&acc](const char *l, uint64_t r, const char *ref, const char *file, unsigned line) {
    if (acc) {
      UInt<256> l256 = UInt<256>::fromHex(l);
      l256.mul64(r);
      acc = cmphex(l256, ref, file, line);
    }
  };

  // ========== Test 1: Zero multiplications ==========
  check("0", 0ull, "0", __FILE__, __LINE__);
  check("0", 1ull, "0", __FILE__, __LINE__);
  check("0", 0xffffffffffffffffull, "0", __FILE__, __LINE__);
  check("1", 0ull, "0", __FILE__, __LINE__);
  check("ffffffffffffffff", 0ull, "0", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0ull, "0", __FILE__, __LINE__);

  // ========== Test 2: Identity multiplications (multiply by 1) ==========
  check("1", 1ull, "1", __FILE__, __LINE__);
  check("ffffffffffffffff", 1ull, "ffffffffffffffff", __FILE__, __LINE__);
  check("123456789abcdef0", 1ull, "123456789abcdef0", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 1ull, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);

  // ========== Test 3: Multiply by 2 ==========
  check("1", 2ull, "2", __FILE__, __LINE__);
  check("ffffffffffffffff", 2ull, "1fffffffffffffffe", __FILE__, __LINE__);
  check("8000000000000000", 2ull, "10000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 2ull, "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe", __FILE__, __LINE__);

  // ========== Test 4: Single limb multiplications ==========
  check("2", 3ull, "6", __FILE__, __LINE__);
  check("ff", 0xffull, "fe01", __FILE__, __LINE__);
  check("100", 0x100ull, "10000", __FILE__, __LINE__);
  check("deadbeef", 0xcafebabeull, "b092ab7b88cf5b62", __FILE__, __LINE__);

  // ========== Test 5: Max 64-bit value multiplications ==========
  check("1", 0xffffffffffffffffull, "ffffffffffffffff", __FILE__, __LINE__);
  check("2", 0xffffffffffffffffull, "1fffffffffffffffe", __FILE__, __LINE__);
  check("ffffffffffffffff", 0xffffffffffffffffull, "fffffffffffffffe0000000000000001", __FILE__, __LINE__);

  // ========== Test 6: Cross-limb carry propagation ==========
  check("10000000000000000", 2ull, "20000000000000000", __FILE__, __LINE__);
  check("10000000000000000", 0xffffffffffffffffull, "ffffffffffffffff0000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffff0000000000000000", 2ull, "1fffffffffffffffe0000000000000000", __FILE__, __LINE__);

  // ========== Test 7: Multi-limb values ==========
  check("ffffffffffffffffffffffffffffffff", 2ull, "1fffffffffffffffffffffffffffffffe", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffff", 0xffffffffffffffffull, "fffffffffffffffeffffffffffffffff0000000000000001", __FILE__, __LINE__);
  check("123456789abcdef0123456789abcdef0", 0x10ull, "123456789abcdef0123456789abcdef00", __FILE__, __LINE__);

  // ========== Test 8: Full 256-bit value multiplications ==========
  check("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0xdeadbeefdeadbeefull, "eeffffffffffffffffffffffffffffffffffffffffffffff2152411021524111", __FILE__, __LINE__);
  check("0000000000000001ffffffffffffffffffffffffffffffffffffffffffffffff", 0x2ull, "3fffffffffffffffffffffffffffffffffffffffffffffffe", __FILE__, __LINE__);

  // ========== Test 9: Powers of 2 multiplier ==========
  check("1", 0x10ull, "10", __FILE__, __LINE__);
  check("1", 0x100ull, "100", __FILE__, __LINE__);
  check("1", 0x10000ull, "10000", __FILE__, __LINE__);
  check("1", 0x100000000ull, "100000000", __FILE__, __LINE__);
  check("1", 0x8000000000000000ull, "8000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffff", 0xffffffffffffffffull, "fffffffffffffffe0000000000000001", __FILE__, __LINE__);

  // ========== Test 10: Specific patterns ==========
  check("aaaaaaaaaaaaaaaa", 2ull, "15555555555555554", __FILE__, __LINE__);
  check("5555555555555555", 3ull, "ffffffffffffffff", __FILE__, __LINE__);
  check("8000000000000000", 0x8000000000000000ull, "40000000000000000000000000000000", __FILE__, __LINE__);

  // ========== Test 11: Edge case with carry chain ==========
  check("ffffffffffffffff00000000000000000000000000000000", 0xffffffffffffffffull, "fffffffffffffffe000000000000000100000000000000000000000000000000", __FILE__, __LINE__);

  return acc;
}

bool testUintDivmod()
{
  bool acc = true;

  // Helper lambda: performs divmod and checks both quotient and remainder
  auto check = [&acc](const char *dividend, const char *divisor,
                      const char *expectedQ, const char *expectedR,
                      const char *file, unsigned line) {
    if (acc) {
      UInt<256> u = UInt<256>::fromHex(dividend);
      UInt<256> v = UInt<256>::fromHex(divisor);
      UInt<256> q, r;
      u.divmod(v, &q, &r);

      bool qOk = cmphex(q, expectedQ, file, line);
      bool rOk = cmphex(r, expectedR, file, line);

      if (!qOk) {
        fprintf(stderr, "   quotient mismatch for %s / %s\n", dividend, divisor);
      }
      if (!rOk) {
        fprintf(stderr, "   remainder mismatch for %s / %s\n", dividend, divisor);
      }

      acc = qOk && rOk;
    }
  };

  // ========== Test 1: Dividend is zero ==========
  // 0 / anything = 0, remainder 0
  check("0", "1", "0", "0", __FILE__, __LINE__);
  check("0", "ffffffffffffffff", "0", "0", __FILE__, __LINE__);
  check("0", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "0", "0", __FILE__, __LINE__);

  // ========== Test 2: Dividend < Divisor ==========
  // quotient = 0, remainder = dividend
  check("1", "2", "0", "1", __FILE__, __LINE__);
  check("ff", "100", "0", "ff", __FILE__, __LINE__);
  check("ffffffffffffffff", "10000000000000000", "0", "ffffffffffffffff", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffffffffffff",
        "0", "ffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);

  // ========== Test 3: Simple single-limb divisions ==========
  // 10 / 2 = 5, r = 0
  check("a", "2", "5", "0", __FILE__, __LINE__);
  // 10 / 3 = 3, r = 1
  check("a", "3", "3", "1", __FILE__, __LINE__);
  // 100 / 7 = 14 (0xe), r = 2
  check("64", "7", "e", "2", __FILE__, __LINE__);
  // 255 / 16 = 15, r = 15
  check("ff", "10", "f", "f", __FILE__, __LINE__);

  // ========== Test 4: 64-bit boundary cases ==========
  // 0xffffffffffffffff / 1 = 0xffffffffffffffff, r = 0
  check("ffffffffffffffff", "1", "ffffffffffffffff", "0", __FILE__, __LINE__);
  // 0xffffffffffffffff / 2 = 0x7fffffffffffffff, r = 1
  check("ffffffffffffffff", "2", "7fffffffffffffff", "1", __FILE__, __LINE__);
  // 0xffffffffffffffff / 0xffffffffffffffff = 1, r = 0
  check("ffffffffffffffff", "ffffffffffffffff", "1", "0", __FILE__, __LINE__);
  // 0xffffffffffffffff / 0xfffffffffffffffe = 1, r = 1
  check("ffffffffffffffff", "fffffffffffffffe", "1", "1", __FILE__, __LINE__);

  // ========== Test 5: Two-limb dividend, one-limb divisor ==========
  // 0x10000000000000000 / 2 = 0x8000000000000000, r = 0
  check("10000000000000000", "2", "8000000000000000", "0", __FILE__, __LINE__);
  // 0x10000000000000001 / 2 = 0x8000000000000000, r = 1
  check("10000000000000001", "2", "8000000000000000", "1", __FILE__, __LINE__);
  // 0xffffffffffffffffffffffffffffffff / 0xffffffffffffffff
  // = 0x10000000000000001, r = 0
  check("ffffffffffffffffffffffffffffffff", "ffffffffffffffff",
        "10000000000000001", "0", __FILE__, __LINE__);

  // ========== Test 6: Multi-limb division with normalization (s > 0) ==========
  // Divisor with leading zeros requiring shift
  // 0x123456789abcdef0123456789abcdef0 / 0x123456789abcdef0
  // High limb of divisor is 0x123456789abcdef0, needs normalization
  check("123456789abcdef0123456789abcdef0", "123456789abcdef0",
        "10000000000000001", "0", __FILE__, __LINE__);

  // ========== Test 7: Division without normalization (s = 0) ==========
  // Divisor with MSB already set (no shift needed)
  // 0x8000000000000000 has MSB set
  check("ffffffffffffffffffffffffffffffff", "8000000000000000",
        "1ffffffffffffffff", "7fffffffffffffff", __FILE__, __LINE__);

  // ========== Test 8: qhat estimation edge cases ==========
  // Case where ujn >= v[n-1], forcing qhat = UINT64_MAX - 1
  // 0xffffffffffffffff0000000000000000 / 0xffffffffffffffff
  // = 0x10000000000000000, r = 0
  check("ffffffffffffffff0000000000000000", "ffffffffffffffff",
        "10000000000000000", "0", __FILE__, __LINE__);

  // ========== Test 9: Knuth D3 correction cases ==========
  // These require careful construction to trigger qhat correction
  // 0x7fffffffffffffff8000000000000000 / 0x8000000000000000
  check("7fffffffffffffff8000000000000000", "8000000000000000",
        "ffffffffffffffff", "0", __FILE__, __LINE__);

  // ========== Test 10: Full 256-bit operations ==========
  // Max 256-bit value / 2
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "2",
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "1", __FILE__, __LINE__);

  // Max 256-bit value / max 256-bit value = 1, r = 0
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "1", "0", __FILE__, __LINE__);

  // Max 256-bit value / (max 256-bit value - 1) = 1, r = 1
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe",
        "1", "1", __FILE__, __LINE__);

  // ========== Test 11: Large dividend / small divisor ==========
  // 0x100000000000000000000000000000000 / 0x10 = 0x10000000000000000000000000000000
  check("100000000000000000000000000000000", "10",
        "10000000000000000000000000000000", "0", __FILE__, __LINE__);

  // ========== Test 12: Multi-limb dividend and divisor ==========
  // 128-bit / 128-bit
  check("ffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffffffffffff",
        "1", "0", __FILE__, __LINE__);

  check("ffffffffffffffffffffffffffffffff", "80000000000000000000000000000000",
        "1", "7fffffffffffffffffffffffffffffff", __FILE__, __LINE__);

  // ========== Test 13: Specific division with known result ==========
  // 1000 / 7 = 142 (0x8e), r = 6
  check("3e8", "7", "8e", "6", __FILE__, __LINE__);

  // 123456789 (0x75bcd15) / 12345 (0x3039) = 10000 (0x2710), r = 6789 (0x1a85)
  check("75bcd15", "3039", "2710", "1a85", __FILE__, __LINE__);

  // ========== Test 14: Power of 2 divisions ==========
  check("ffffffffffffffff", "10000000000000000", "0", "ffffffffffffffff", __FILE__, __LINE__);
  check("10000000000000000", "10000000000000000", "1", "0", __FILE__, __LINE__);
  check("20000000000000000", "10000000000000000", "2", "0", __FILE__, __LINE__);
  check("30000000000000001", "10000000000000000", "3", "1", __FILE__, __LINE__);

  // ========== Test 15: Cases triggering D6 borrow correction ==========
  // These are carefully constructed to cause the subtract to go negative
  // and require adding the divisor back
  // This is rare but needs to be tested
  check("8000000000000000000000000000000000000000000000000000000000001234", "800000000000000000000000000000000000000000000002",
        "ffffffffffffffff", "7ffffffffffffffffffffffffffffffe0000000000001236", __FILE__, __LINE__);

  // ========== Test 16: Verify quotient * divisor + remainder = dividend ==========
  // This is a sanity check - we compute q*d + r and verify it equals original
  {
    UInt<256> dividend = UInt<256>::fromHex("deadbeefcafebabe123456789abcdef0");
    UInt<256> divisor = UInt<256>::fromHex("fedcba9876543210");
    UInt<256> q, r;
    dividend.divmod(divisor, &q, &r);

    // Verify: q * divisor + r == dividend
    UInt<256> check_val = q;
    check_val *= divisor;
    check_val += r;

    if (!cmphex(check_val, dividend.getHex(false), __FILE__, __LINE__)) {
      fprintf(stderr, "   q*d+r != dividend verification failed\n");
      acc = false;
    }
  }

  // ========== Test 17: Division resulting in multi-limb quotient ==========
  // Large dividend / small divisor = large quotient
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "ff",
        "10101010101010101010101010101010101010101010101010101010101", "0", __FILE__, __LINE__);

  // ========== Test 18: Edge case - dividend exactly divisible ==========
  // 0x100 * 0xff = 0xff00
  check("ff00", "100", "ff", "0", __FILE__, __LINE__);
  check("ff00", "ff", "100", "0", __FILE__, __LINE__);

  // ========== Test 19: Three-limb operations ==========
  check("ffffffffffffffffffffffffffffffffffffffff", "ffffffffffffffff",
        "1000000000000000100000000", "ffffffff", __FILE__, __LINE__);

  // ========== Test 20: Four-limb dividend / two-limb divisor ==========
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "ffffffffffffffffffffffffffffffff",
        "100000000000000000000000000000001", "0", __FILE__, __LINE__);

  // ========== Test 21: Null pointer handling ==========
  // Test that passing nullptr for quotient or remainder doesn't crash
  {
    UInt<256> u = UInt<256>::fromHex("100");
    UInt<256> v = UInt<256>::fromHex("10");
    UInt<256> q, r;

    // Only get quotient
    u.divmod(v, &q, nullptr);
    if (!cmphex(q, "10", __FILE__, __LINE__)) {
      fprintf(stderr, "   nullptr remainder test failed\n");
      acc = false;
    }

    // Only get remainder
    u.divmod(v, nullptr, &r);
    if (!cmphex(r, "0", __FILE__, __LINE__)) {
      fprintf(stderr, "   nullptr quotient test failed\n");
      acc = false;
    }
  }

  // ========== Test 22: Various bit patterns ==========
  check("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "5555555555555555",
        "2000000000000000200000000000000020000000000000002", "0", __FILE__, __LINE__);

  check("5555555555555555555555555555555555555555555555555555555555555555",
        "aaaaaaaaaaaaaaaa",
        "800000000000000080000000000000008000000000000000", "5555555555555555", __FILE__, __LINE__);

  // ========== Test 23: More D6 rollback scenarios ==========
  // These cases are designed to trigger the rare add-back correction in Knuth D6
  // The condition for D6 is: when the initial subtract goes negative (borrow != 0)

  // Case: divisor starting with 0x80... (already normalized, s=0)
  check("8000000000000000ffffffffffffffffffffffffffffffff", "80000000000000000000000000000001",
        "10000000000000001", "7ffffffffffffffefffffffffffffffe", __FILE__, __LINE__);

  // ========== Test 24: Single limb divisor edge cases ==========
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "1",
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "0", __FILE__, __LINE__);

  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "3",
        "5555555555555555555555555555555555555555555555555555555555555555", "0", __FILE__, __LINE__);

  // ========== Test 25: Divisor with different limb counts ==========
  // 3-limb divisor
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "ffffffffffffffffffffffffffffffffffffffffffff",
        "100000000000000000000", "ffffffffffffffffffff", __FILE__, __LINE__);

  // ========== Test 26: Edge cases for qhat estimation ==========
  // Case where ujn == v[n-1] exactly
  check("ffffffffffffffff00000000000000000000000000000000", "ffffffffffffffff0000000000000000",
        "10000000000000000", "0", __FILE__, __LINE__);

  // ========== Test 27: Verification with random-like patterns ==========
  check("123456789abcdef0fedcba9876543210", "123456789abcdef0",
        "1000000000000000e", "f0", __FILE__, __LINE__);

  check("deadbeefcafebabedeadbeefcafebabe", "deadbeefcafebabe",
        "10000000000000001", "0", __FILE__, __LINE__);

  // ========== Test 28: Cases near powers of 2 ==========
  check("10000000000000000", "ffffffffffffffff", "1", "1", __FILE__, __LINE__);
  check("fffffffffffffffe", "ffffffffffffffff", "0", "fffffffffffffffe", __FILE__, __LINE__);
  check("ffffffffffffffff", "fffffffffffffffe", "1", "1", __FILE__, __LINE__);

  // ========== Test 29: Dividend equals divisor ==========
  check("deadbeef", "deadbeef", "1", "0", __FILE__, __LINE__);
  check("123456789abcdef0123456789abcdef0", "123456789abcdef0123456789abcdef0",
        "1", "0", __FILE__, __LINE__);

  // ========== Test 30: Large scale division ==========
  // 256-bit / 128-bit producing 128+ bit quotient
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "10000000000000000",
        "ffffffffffffffffffffffffffffffffffffffffffffffff", "ffffffffffffffff", __FILE__, __LINE__);

  // ========== Test 31: Special cases for D6 add-back (rollback) ==========
  // The D6 add-back is very rare. It happens when qhat overestimates by 1 AFTER
  // the D3 correction loop exits. This requires careful construction.
  // According to Knuth, this happens with probability at most 2/b (where b = 2^64)
  //
  // One known trigger pattern: when D3 correction is just barely avoided
  // and the actual subtraction overflows

  // ========== Test 32: Edge case where divisorLimbs == dividendLimbs ==========
  check("ffffffffffffffff", "ffffffffffffffff", "1", "0", __FILE__, __LINE__);
  check("80000000000000000000000000000000", "80000000000000000000000000000000", "1", "0", __FILE__, __LINE__);
  check("80000000000000000000000000000001", "80000000000000000000000000000000", "1", "1", __FILE__, __LINE__);

  // ========== Test 33: Normalization shift edge cases ==========
  // s = 0 (MSB already set)
  check("ffffffffffffffff0000000000000000", "8000000000000000",
        "1fffffffffffffffe", "0", __FILE__, __LINE__);

  // s = 63 (only LSB set in high limb)
  check("ffffffffffffffff0000000000000000", "10000000000000000",
        "ffffffffffffffff", "0", __FILE__, __LINE__);

  // ========== Test 34: Multi-iteration quotient calculation ==========
  // Testing the main loop with multiple quotient digits
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "1000000000000000000000000",
        "ffffffffffffffffffffffffffffffff", "ffffffffffffffffffffffff", __FILE__, __LINE__);

  // ========== Test 35: Verify invariant: q*d + r == original ==========
  // More verification tests
  {
    const char* testCases[][2] = {
                                  {"abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789", "fedcba9876543210fedcba98"},
                                  {"111111111111111111111111111111111111111111111111", "2222222222222222"},
                                  {"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee", "dddddddddddddddd"},
                                  };

    for (const auto& tc : testCases) {
      UInt<256> dividend = UInt<256>::fromHex(tc[0]);
      UInt<256> divisor = UInt<256>::fromHex(tc[1]);
      UInt<256> q, r;
      dividend.divmod(divisor, &q, &r);

      // Verify: q * divisor + r == dividend
      UInt<256> check_val = q;
      check_val *= divisor;
      check_val += r;

      if (!cmphex(check_val, dividend.getHex(false), __FILE__, __LINE__)) {
        fprintf(stderr, "   q*d+r != dividend verification failed for %s / %s\n", tc[0], tc[1]);
        acc = false;
      }
    }
  }

  return acc;
}

bool testUintAdd64()
{
  bool acc = true;

  auto check = [&acc](const char *value, uint64_t addend,
                      const char *expected, const char *file, unsigned line) {
    if (acc) {
      UInt<256> v = UInt<256>::fromHex(value);
      v += addend;
      acc = cmphex(v, expected, file, line);
      if (!acc) {
        fprintf(stderr, "   add64 mismatch for %s + %llx\n", value, (unsigned long long)addend);
      }
    }
  };

  // Zero additions
  check("0", 0ull, "0", __FILE__, __LINE__);
  check("0", 1ull, "1", __FILE__, __LINE__);
  check("1", 0ull, "1", __FILE__, __LINE__);
  check("ffffffffffffffff", 0ull, "ffffffffffffffff", __FILE__, __LINE__);

  // Simple additions
  check("1", 1ull, "2", __FILE__, __LINE__);
  check("1", 0xfffffffffffffffeull, "ffffffffffffffff", __FILE__, __LINE__);
  check("ff", 1ull, "100", __FILE__, __LINE__);
  check("deadbeef", 0x11111111ull, "efbed000", __FILE__, __LINE__);

  // Carry propagation within first limb
  check("ffffffffffffffff", 1ull, "10000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffff", 2ull, "10000000000000001", __FILE__, __LINE__);
  check("fffffffffffffffe", 2ull, "10000000000000000", __FILE__, __LINE__);

  // Max uint64_t addition
  check("0", 0xffffffffffffffffull, "ffffffffffffffff", __FILE__, __LINE__);
  check("1", 0xffffffffffffffffull, "10000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffff", 0xffffffffffffffffull, "1fffffffffffffffe", __FILE__, __LINE__);

  // Multi-limb carry propagation
  check("ffffffffffffffffffffffffffffffff", 1ull, "100000000000000000000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffff", 1ull, "1000000000000000000000000000000000000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 1ull, "0", __FILE__, __LINE__); // overflow wraps

  // Two-limb values
  check("10000000000000000", 1ull, "10000000000000001", __FILE__, __LINE__);
  check("10000000000000000", 0xffffffffffffffffull, "1ffffffffffffffff", __FILE__, __LINE__);

  // Large values
  check("123456789abcdef0123456789abcdef0", 0x10ull, "123456789abcdef0123456789abcdf00", __FILE__, __LINE__);

  return acc;
}

bool testUintAdd()
{
  bool acc = true;

  auto check = [&acc](const char *left, const char *right,
                      const char *expected, const char *file, unsigned line) {
    if (acc) {
      UInt<256> l = UInt<256>::fromHex(left);
      UInt<256> r = UInt<256>::fromHex(right);
      l += r;
      acc = cmphex(l, expected, file, line);
      if (!acc) {
        fprintf(stderr, "   add mismatch for %s + %s\n", left, right);
      }
    }
  };

  // Zero additions
  check("0", "0", "0", __FILE__, __LINE__);
  check("0", "1", "1", __FILE__, __LINE__);
  check("1", "0", "1", __FILE__, __LINE__);
  check("ffffffffffffffff", "0", "ffffffffffffffff", __FILE__, __LINE__);

  // Simple additions
  check("1", "1", "2", __FILE__, __LINE__);
  check("1", "fffffffffffffffe", "ffffffffffffffff", __FILE__, __LINE__);
  check("ff", "1", "100", __FILE__, __LINE__);
  check("deadbeef", "11111111", "efbed000", __FILE__, __LINE__);

  // Carry propagation
  check("ffffffffffffffff", "1", "10000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffff", "ffffffffffffffff", "1fffffffffffffffe", __FILE__, __LINE__);

  // Multi-limb additions
  check("10000000000000000", "10000000000000000", "20000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffff0000000000000000", "10000000000000000", "100000000000000000000000000000000", __FILE__, __LINE__);

  // Full 256-bit additions
  check("ffffffffffffffffffffffffffffffff", "1", "100000000000000000000000000000000", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "1", "0", __FILE__, __LINE__); // overflow

  // Symmetric test
  check("123456789abcdef0", "fedcba9876543210", "11111111111111100", __FILE__, __LINE__);

  // Large operands
  check("8000000000000000000000000000000000000000000000000000000000000000",
        "8000000000000000000000000000000000000000000000000000000000000000",
        "0", __FILE__, __LINE__); // overflow wraps to 0

  check("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "1",
        "8000000000000000000000000000000000000000000000000000000000000000", __FILE__, __LINE__);

  // Multi-limb carry chain
  check("ffffffffffffffff00000000000000000000000000000000",
        "ffffffffffffffff00000000000000000000000000000001",
        "1fffffffffffffffe00000000000000000000000000000001", __FILE__, __LINE__);

  return acc;
}

bool testUintSub64()
{
  bool acc = true;

  auto check = [&acc](const char *value, uint64_t subtrahend,
                      const char *expected, const char *file, unsigned line) {
    if (acc) {
      UInt<256> v = UInt<256>::fromHex(value);
      v -= subtrahend;
      acc = cmphex(v, expected, file, line);
      if (!acc) {
        fprintf(stderr, "   sub64 mismatch for %s - %llx\n", value, (unsigned long long)subtrahend);
      }
    }
  };

  // Zero subtractions
  check("0", 0ull, "0", __FILE__, __LINE__);
  check("1", 0ull, "1", __FILE__, __LINE__);
  check("ffffffffffffffff", 0ull, "ffffffffffffffff", __FILE__, __LINE__);

  // Simple subtractions
  check("1", 1ull, "0", __FILE__, __LINE__);
  check("2", 1ull, "1", __FILE__, __LINE__);
  check("100", 1ull, "ff", __FILE__, __LINE__);
  check("ffffffffffffffff", 1ull, "fffffffffffffffe", __FILE__, __LINE__);
  check("ffffffffffffffff", 0xffffffffffffffffull, "0", __FILE__, __LINE__);

  // Borrow propagation
  check("10000000000000000", 1ull, "ffffffffffffffff", __FILE__, __LINE__);
  check("10000000000000001", 2ull, "ffffffffffffffff", __FILE__, __LINE__);

  // Multi-limb borrow propagation
  check("100000000000000000000000000000000", 1ull, "ffffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  check("1000000000000000000000000000000000000000000000000", 1ull, "ffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);

  // Underflow (wraps around)
  check("0", 1ull, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  check("1", 2ull, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);

  // Large values
  check("123456789abcdef0123456789abcdef0", 0x10ull, "123456789abcdef0123456789abcdee0", __FILE__, __LINE__);

  // Subtract max uint64
  check("ffffffffffffffff", 0xffffffffffffffffull, "0", __FILE__, __LINE__);
  check("10000000000000000", 0xffffffffffffffffull, "1", __FILE__, __LINE__);
  check("1ffffffffffffffff", 0xffffffffffffffffull, "10000000000000000", __FILE__, __LINE__);

  return acc;
}

bool testUintSub()
{
  bool acc = true;

  auto check = [&acc](const char *left, const char *right,
                      const char *expected, const char *file, unsigned line) {
    if (acc) {
      UInt<256> l = UInt<256>::fromHex(left);
      UInt<256> r = UInt<256>::fromHex(right);
      l -= r;
      acc = cmphex(l, expected, file, line);
      if (!acc) {
        fprintf(stderr, "   sub mismatch for %s - %s\n", left, right);
      }
    }
  };

  // Zero subtractions
  check("0", "0", "0", __FILE__, __LINE__);
  check("1", "0", "1", __FILE__, __LINE__);
  check("ffffffffffffffff", "0", "ffffffffffffffff", __FILE__, __LINE__);

  // Simple subtractions
  check("1", "1", "0", __FILE__, __LINE__);
  check("2", "1", "1", __FILE__, __LINE__);
  check("100", "1", "ff", __FILE__, __LINE__);
  check("ffffffffffffffff", "ffffffffffffffff", "0", __FILE__, __LINE__);

  // Borrow propagation
  check("10000000000000000", "1", "ffffffffffffffff", __FILE__, __LINE__);
  check("10000000000000000", "ffffffffffffffff", "1", __FILE__, __LINE__);

  // Multi-limb subtractions
  check("20000000000000000", "10000000000000000", "10000000000000000", __FILE__, __LINE__);
  check("100000000000000000000000000000000", "1", "ffffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  check("100000000000000000000000000000000", "ffffffffffffffff", "ffffffffffffffff0000000000000001", __FILE__, __LINE__);

  // Full 256-bit subtractions
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "0", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "1", "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe", __FILE__, __LINE__);

  // Underflow (wraps around)
  check("0", "1", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  check("1", "2", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  check("0", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "1", __FILE__, __LINE__);

  // Symmetric test with add
  check("11111111111111100", "fedcba9876543210", "123456789abcdef0", __FILE__, __LINE__);

  // Multi-limb borrow chain
  check("10000000000000000000000000000000000000000000000000000000000000000",
        "1",
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);


  // A narrower subtrahend: the borrow has to travel through the limbs above it
  if (acc) {
    UInt<256> l = UInt<256>::fromHex("1000000000000000000000000000000000000000000000000");
    l -= UInt<128>::fromHex("1");
    acc = cmphex(l, "ffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  }

  return acc;
}
bool testUintDiv64()
{
  bool acc = true;

  auto check = [&acc](const char *dividend, uint64_t divisor,
                      const char *expected, const char *file, unsigned line) {
    if (acc) {
      UInt<256> v = UInt<256>::fromHex(dividend);
      v /= divisor;
      acc = cmphex(v, expected, file, line);
      if (!acc) {
        fprintf(stderr, "   div64 mismatch for %s / %llx\n", dividend, (unsigned long long)divisor);
      }
    }
  };

  // Division by 1
  check("0", 1ull, "0", __FILE__, __LINE__);
  check("1", 1ull, "1", __FILE__, __LINE__);
  check("ffffffffffffffff", 1ull, "ffffffffffffffff", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 1ull,
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);

  // Simple divisions
  check("2", 2ull, "1", __FILE__, __LINE__);
  check("a", 2ull, "5", __FILE__, __LINE__);
  check("a", 3ull, "3", __FILE__, __LINE__);
  check("64", 7ull, "e", __FILE__, __LINE__); // 100 / 7 = 14
  check("ff", 0x10ull, "f", __FILE__, __LINE__); // 255 / 16 = 15

  // 64-bit boundary cases
  check("ffffffffffffffff", 2ull, "7fffffffffffffff", __FILE__, __LINE__);
  check("ffffffffffffffff", 0xffffffffffffffffull, "1", __FILE__, __LINE__);
  check("fffffffffffffffe", 0xffffffffffffffffull, "0", __FILE__, __LINE__);

  // Two-limb dividend
  check("10000000000000000", 2ull, "8000000000000000", __FILE__, __LINE__);
  check("10000000000000000", 0xffffffffffffffffull, "1", __FILE__, __LINE__);
  check("1ffffffffffffffff", 2ull, "ffffffffffffffff", __FILE__, __LINE__);

  // Multi-limb dividend
  check("ffffffffffffffffffffffffffffffff", 2ull, "7fffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffff", 0xffffffffffffffffull, "10000000000000001", __FILE__, __LINE__);

  // Full 256-bit divided by small
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 2ull,
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 3ull,
        "5555555555555555555555555555555555555555555555555555555555555555", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0xffull,
        "101010101010101010101010101010101010101010101010101010101010101", __FILE__, __LINE__);

  // Power of 2 divisions
  check("100", 0x10ull, "10", __FILE__, __LINE__);
  check("10000000000000000", 0x10ull, "1000000000000000", __FILE__, __LINE__);
  // Large dividend / max uint64_t (result fits in 256 bits)
  check("fffffffffffffffe0000000000000001", 0xffffffffffffffffull, "ffffffffffffffff", __FILE__, __LINE__);
  // Verify division: quotient * divisor should be <= dividend
  check("deadbeefcafebabe", 0xdeadbeefull, "100000000", __FILE__, __LINE__);
  // Division producing single limb result
  check("ffffffffffffff", 0x10000000000000ull, "f", __FILE__, __LINE__);

  return acc;
}

bool testUintCmp()
{
  bool acc = true;

  auto check = [&acc](const char *left, const char *right,
                      int expectedCmp, bool expectedEq, bool expectedNe,
                      bool expectedLt, bool expectedLe,
                      bool expectedGt, bool expectedGe,
                      const char *file, unsigned line) {
    if (!acc) return;

    UInt<256> l = UInt<256>::fromHex(left);
    UInt<256> r = UInt<256>::fromHex(right);

    int cmpResult = l.cmp(r);
    bool eqResult = (l == r);
    bool neResult = (l != r);
    bool ltResult = (l < r);
    bool leResult = (l <= r);
    bool gtResult = (l > r);
    bool geResult = (l >= r);

    if (cmpResult != expectedCmp) {
      fprintf(stderr, " * %s:%u cmp mismatch for %s vs %s: got %d, expected %d\n",
              file, line, left, right, cmpResult, expectedCmp);
      acc = false;
    }
    if (eqResult != expectedEq) {
      fprintf(stderr, " * %s:%u operator== mismatch for %s vs %s\n", file, line, left, right);
      acc = false;
    }
    if (neResult != expectedNe) {
      fprintf(stderr, " * %s:%u operator!= mismatch for %s vs %s\n", file, line, left, right);
      acc = false;
    }
    if (ltResult != expectedLt) {
      fprintf(stderr, " * %s:%u operator< mismatch for %s vs %s\n", file, line, left, right);
      acc = false;
    }
    if (leResult != expectedLe) {
      fprintf(stderr, " * %s:%u operator<= mismatch for %s vs %s\n", file, line, left, right);
      acc = false;
    }
    if (gtResult != expectedGt) {
      fprintf(stderr, " * %s:%u operator> mismatch for %s vs %s\n", file, line, left, right);
      acc = false;
    }
    if (geResult != expectedGe) {
      fprintf(stderr, " * %s:%u operator>= mismatch for %s vs %s\n", file, line, left, right);
      acc = false;
    }
  };

  // check(left, right, cmp, ==, !=, <, <=, >, >=)

  // Equal values
  check("0", "0", 0, true, false, false, true, false, true, __FILE__, __LINE__);
  check("1", "1", 0, true, false, false, true, false, true, __FILE__, __LINE__);
  check("ffffffffffffffff", "ffffffffffffffff", 0, true, false, false, true, false, true, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        0, true, false, false, true, false, true, __FILE__, __LINE__);
  check("deadbeefcafebabe", "deadbeefcafebabe", 0, true, false, false, true, false, true, __FILE__, __LINE__);

  // Left < Right (single limb)
  check("0", "1", -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("1", "2", -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("fe", "ff", -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("7fffffffffffffff", "8000000000000000", -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("fffffffffffffffe", "ffffffffffffffff", -1, false, true, true, true, false, false, __FILE__, __LINE__);

  // Left > Right (single limb)
  check("1", "0", 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("2", "1", 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("ff", "fe", 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("8000000000000000", "7fffffffffffffff", 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("ffffffffffffffff", "fffffffffffffffe", 1, false, true, false, false, true, true, __FILE__, __LINE__);

  // Multi-limb: different in high limb
  check("ffffffffffffffff", "10000000000000000", -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("10000000000000000", "ffffffffffffffff", 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("1ffffffffffffffff", "20000000000000000", -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("20000000000000000", "1ffffffffffffffff", 1, false, true, false, false, true, true, __FILE__, __LINE__);

  // Multi-limb: same high limb, different low limb
  check("10000000000000000", "10000000000000001", -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("10000000000000001", "10000000000000000", 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("ffffffffffffffff0000000000000000", "ffffffffffffffff0000000000000001",
        -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("ffffffffffffffff0000000000000001", "ffffffffffffffff0000000000000000",
        1, false, true, false, false, true, true, __FILE__, __LINE__);

  // Multi-limb: 3+ limbs
  check("100000000000000000000000000000000", "100000000000000000000000000000001",
        -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("100000000000000000000000000000001", "100000000000000000000000000000000",
        1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffff", "100000000000000000000000000000000",
        -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("100000000000000000000000000000000", "ffffffffffffffffffffffffffffffff",
        1, false, true, false, false, true, true, __FILE__, __LINE__);

  // Full 256-bit comparisons
  check("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe",
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe",
        1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("8000000000000000000000000000000000000000000000000000000000000000",
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "8000000000000000000000000000000000000000000000000000000000000000",
        -1, false, true, true, true, false, false, __FILE__, __LINE__);

  // Zero comparisons
  check("0", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "0",
        1, false, true, false, false, true, true, __FILE__, __LINE__);

  // Edge: leading zeros in hex don't affect value
  check("0000000000000001", "1", 0, true, false, false, true, false, true, __FILE__, __LINE__);
  check("00000000000000000000000000000000ffffffffffffffff", "ffffffffffffffff",
        0, true, false, false, true, false, true, __FILE__, __LINE__);

  return acc;
}

bool testUintCmp64()
{
  bool acc = true;

  auto check = [&acc](const char *left, uint64_t right,
                      int expectedCmp, bool expectedEq, bool expectedNe,
                      bool expectedLt, bool expectedLe,
                      bool expectedGt, bool expectedGe,
                      const char *file, unsigned line) {
    if (!acc) return;

    UInt<256> l = UInt<256>::fromHex(left);

    int cmpResult = l.cmp64(right);
    bool eqResult = (l == right);
    bool neResult = (l != right);
    bool ltResult = (l < right);
    bool leResult = (l <= right);
    bool gtResult = (l > right);
    bool geResult = (l >= right);

    if (cmpResult != expectedCmp) {
      fprintf(stderr, " * %s:%u cmp64 mismatch for %s vs %llx: got %d, expected %d\n",
              file, line, left, (unsigned long long)right, cmpResult, expectedCmp);
      acc = false;
    }
    if (eqResult != expectedEq) {
      fprintf(stderr, " * %s:%u operator== mismatch for %s vs %llx\n",
              file, line, left, (unsigned long long)right);
      acc = false;
    }
    if (neResult != expectedNe) {
      fprintf(stderr, " * %s:%u operator!= mismatch for %s vs %llx\n",
              file, line, left, (unsigned long long)right);
      acc = false;
    }
    if (ltResult != expectedLt) {
      fprintf(stderr, " * %s:%u operator< mismatch for %s vs %llx\n",
              file, line, left, (unsigned long long)right);
      acc = false;
    }
    if (leResult != expectedLe) {
      fprintf(stderr, " * %s:%u operator<= mismatch for %s vs %llx\n",
              file, line, left, (unsigned long long)right);
      acc = false;
    }
    if (gtResult != expectedGt) {
      fprintf(stderr, " * %s:%u operator> mismatch for %s vs %llx\n",
              file, line, left, (unsigned long long)right);
      acc = false;
    }
    if (geResult != expectedGe) {
      fprintf(stderr, " * %s:%u operator>= mismatch for %s vs %llx\n",
              file, line, left, (unsigned long long)right);
      acc = false;
    }
  };

  // check(left, right, cmp, ==, !=, <, <=, >, >=)

  // Equal values
  check("0", 0ull, 0, true, false, false, true, false, true, __FILE__, __LINE__);
  check("1", 1ull, 0, true, false, false, true, false, true, __FILE__, __LINE__);
  check("ff", 0xffull, 0, true, false, false, true, false, true, __FILE__, __LINE__);
  check("ffffffffffffffff", 0xffffffffffffffffull, 0, true, false, false, true, false, true, __FILE__, __LINE__);
  check("deadbeef", 0xdeadbeefull, 0, true, false, false, true, false, true, __FILE__, __LINE__);

  // Left < Right (single limb only)
  check("0", 1ull, -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("1", 2ull, -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("fe", 0xffull, -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("7fffffffffffffff", 0x8000000000000000ull, -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("fffffffffffffffe", 0xffffffffffffffffull, -1, false, true, true, true, false, false, __FILE__, __LINE__);

  // Left > Right (single limb)
  check("1", 0ull, 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("2", 1ull, 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("ff", 0xfeull, 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("8000000000000000", 0x7fffffffffffffffull, 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("ffffffffffffffff", 0xfffffffffffffffeull, 1, false, true, false, false, true, true, __FILE__, __LINE__);

  // Multi-limb left > any uint64_t right
  check("10000000000000000", 0ull, 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("10000000000000000", 0xffffffffffffffffull, 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("10000000000000001", 1ull, 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffff", 0xffffffffffffffffull, 1, false, true, false, false, true, true, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        0xffffffffffffffffull, 1, false, true, false, false, true, true, __FILE__, __LINE__);

  // Edge cases
  check("0", 0xffffffffffffffffull, -1, false, true, true, true, false, false, __FILE__, __LINE__);
  check("ffffffffffffffff", 0ull, 1, false, true, false, false, true, true, __FILE__, __LINE__);

  return acc;
}

bool testUintMod64()
{
  bool acc = true;

  auto check = [&acc](const char *dividend, uint64_t divisor, uint64_t expected,
                      const char *file, unsigned line) {
    if (!acc) return;

    UInt<256> v = UInt<256>::fromHex(dividend);
    uint64_t result = v % divisor;

    if (result != expected) {
      fprintf(stderr, " * %s:%u mod64 mismatch for %s %% %llx: got %llx, expected %llx\n",
              file, line, dividend, (unsigned long long)divisor,
              (unsigned long long)result, (unsigned long long)expected);
      acc = false;
    }
  };

  // Modulo by 1 always returns 0
  check("0", 1ull, 0ull, __FILE__, __LINE__);
  check("1", 1ull, 0ull, __FILE__, __LINE__);
  check("ffffffffffffffff", 1ull, 0ull, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 1ull, 0ull, __FILE__, __LINE__);

  // Simple modulo
  check("0", 2ull, 0ull, __FILE__, __LINE__);
  check("1", 2ull, 1ull, __FILE__, __LINE__);
  check("2", 2ull, 0ull, __FILE__, __LINE__);
  check("3", 2ull, 1ull, __FILE__, __LINE__);
  check("a", 3ull, 1ull, __FILE__, __LINE__);  // 10 % 3 = 1
  check("64", 7ull, 2ull, __FILE__, __LINE__); // 100 % 7 = 2
  check("ff", 0x10ull, 0xfull, __FILE__, __LINE__); // 255 % 16 = 15

  // 64-bit boundary cases
  check("ffffffffffffffff", 2ull, 1ull, __FILE__, __LINE__);
  check("ffffffffffffffff", 0xffffffffffffffffull, 0ull, __FILE__, __LINE__);
  check("fffffffffffffffe", 0xffffffffffffffffull, 0xfffffffffffffffeull, __FILE__, __LINE__);

  // Two-limb dividend
  check("10000000000000000", 2ull, 0ull, __FILE__, __LINE__);
  check("10000000000000001", 2ull, 1ull, __FILE__, __LINE__);
  check("10000000000000000", 0xffffffffffffffffull, 1ull, __FILE__, __LINE__);
  check("1ffffffffffffffff", 2ull, 1ull, __FILE__, __LINE__);

  // Multi-limb dividend
  check("ffffffffffffffffffffffffffffffff", 2ull, 1ull, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffff", 0xffffffffffffffffull, 0ull, __FILE__, __LINE__);

  // Full 256-bit modulo
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 2ull, 1ull, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 3ull, 0ull, __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0xffull, 0ull, __FILE__, __LINE__);

  // Random values (first two: divisor <= 32 bits)
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0x12345678ull, 0x2581b87ull, __FILE__, __LINE__);
  check("abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789", 0xdeadbeefull, 0x505719f5ull, __FILE__, __LINE__);
  check("123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0", 0x123456789abcdef0ull, 0x0ull, __FILE__, __LINE__);
  check("fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210", 0xfedcba9876543210ull, 0x0ull, __FILE__, __LINE__);
  check("af83c3ccd11c313dffc3fc1cae748dca9953b351c0f6b02aa09c0854e2fa225c", 0x8f3c56610d5af1a7ull, 0x6c751c6afda3bedbull, __FILE__, __LINE__);
  check("916d99976c6c690b566a0fb87a73edf8cbe8f53fe45926fc7f19275d2d922c4f", 0x70a24d8672cc03a9ull, 0x155f8d3ec6b4ea50ull, __FILE__, __LINE__);

  // Verify: (q * d) + r == original
  {
    UInt<256> dividend = UInt<256>::fromHex("deadbeefcafebabedeadbeefcafebabe");
    uint64_t divisor = 0xfedcba9876543210ull;
    UInt<256> q = dividend;
    q /= divisor;
    uint64_t r = dividend % divisor;
    q *= divisor;
    q += r;
    if (q != dividend) {
      fprintf(stderr, " * mod64 verification failed: q*d+r != dividend\n");
      acc = false;
    }
  }

  return acc;
}

bool testUintCompact()
{
  bool acc = true;

  // Test uint256SetCompact -> GetCompact roundtrip
  auto checkRoundtrip = [&acc](uint32_t compact, const char *file, unsigned line) {
    if (!acc) return;

    bool negative = false, overflow = false;
    UInt<256> value = uint256Compact(compact, &negative, &overflow);

    if (overflow) return; // Skip overflow cases for roundtrip

    uint32_t result = uint256GetCompact(value);

    // Normalize: SetCompact may produce different but equivalent compact
    UInt<256> check = uint256Compact(result);

    if (value != check) {
      fprintf(stderr, " * %s:%u roundtrip mismatch for compact 0x%08x\n", file, line, compact);
      acc = false;
    }
  };

  // Test uint256SetCompact with known values
  auto checkSetCompact = [&acc](uint32_t compact, const char *expectedHex,
                                 bool expectedNeg, bool expectedOverflow,
                                 const char *file, unsigned line) {
    if (!acc) return;

    bool negative = false, overflow = false;
    UInt<256> value = uint256Compact(compact, &negative, &overflow);

    if (overflow != expectedOverflow) {
      fprintf(stderr, " * %s:%u overflow mismatch for compact 0x%08x: got %d, expected %d\n",
              file, line, compact, overflow, expectedOverflow);
      acc = false;
      return;
    }

    if (negative != expectedNeg) {
      fprintf(stderr, " * %s:%u negative mismatch for compact 0x%08x: got %d, expected %d\n",
              file, line, compact, negative, expectedNeg);
      acc = false;
      return;
    }

    if (!expectedOverflow && value.getHex(false) != expectedHex) {
      fprintf(stderr, " * %s:%u value mismatch for compact 0x%08x: got %s, expected %s\n",
              file, line, compact, value.getHex(false).c_str(), expectedHex);
      acc = false;
    }
  };

  // Test GetCompact with known values
  auto checkGetCompact = [&acc](const char *hex, uint32_t expectedCompact,
                                 const char *file, unsigned line) {
    if (!acc) return;

    UInt<256> value = UInt<256>::fromHex(hex);
    uint32_t result = uint256GetCompact(value);

    if (result != expectedCompact) {
      fprintf(stderr, " * %s:%u GetCompact mismatch for %s: got 0x%08x, expected 0x%08x\n",
              file, line, hex, result, expectedCompact);
      acc = false;
    }
  };

  // Zero
  checkSetCompact(0x00000000, "0", false, false, __FILE__, __LINE__);
  checkGetCompact("0", 0x00000000, __FILE__, __LINE__);

  // Small values (nSize <= 3)
  checkSetCompact(0x01000001, "0", false, false, __FILE__, __LINE__); // size=1, word=1
  checkSetCompact(0x01010000, "1", false, false, __FILE__, __LINE__);
  checkSetCompact(0x017f0000, "7f", false, false, __FILE__, __LINE__);
  checkSetCompact(0x02010000, "100", false, false, __FILE__, __LINE__);
  checkSetCompact(0x027f0000, "7f00", false, false, __FILE__, __LINE__);
  checkSetCompact(0x03010000, "10000", false, false, __FILE__, __LINE__);
  checkSetCompact(0x037fffff, "7fffff", false, false, __FILE__, __LINE__);

  // Larger values (nSize > 3)
  checkSetCompact(0x04010000, "1000000", false, false, __FILE__, __LINE__);
  checkSetCompact(0x04123456, "12345600", false, false, __FILE__, __LINE__);
  checkSetCompact(0x05009234, "92340000", false, false, __FILE__, __LINE__);

  // Bitcoin mainnet genesis block target
  checkSetCompact(0x1d00ffff, "ffff0000000000000000000000000000000000000000000000000000", false, false, __FILE__, __LINE__);

  // Negative flag (0x00800000 bit set AND non-zero word after shift)
  checkSetCompact(0x03800100, "100", true, false, __FILE__, __LINE__); // size=3, word=0x000100

  // Overflow cases
  checkSetCompact(0x23010000, "0", false, true, __FILE__, __LINE__); // nSize = 35 > 34
  checkSetCompact(0x22010000, "0", false, true, __FILE__, __LINE__); // nSize = 34, word = 0x010000 > 0xff
  checkSetCompact(0x21010000, "0", false, true, __FILE__, __LINE__); // nSize = 33, word = 0x010000 > 0xffff
  checkSetCompact(0x21000100, "100000000000000000000000000000000000000000000000000000000000000", false, false, __FILE__, __LINE__); // nSize = 33, word = 0x000100 <= 0xffff, ok

  // Roundtrip tests
  checkRoundtrip(0x1d00ffff, __FILE__, __LINE__);
  checkRoundtrip(0x1b0404cb, __FILE__, __LINE__);
  checkRoundtrip(0x04123456, __FILE__, __LINE__);
  checkRoundtrip(0x01010000, __FILE__, __LINE__);

  // GetCompact tests
  checkGetCompact("1", 0x01010000, __FILE__, __LINE__);
  checkGetCompact("7f", 0x017f0000, __FILE__, __LINE__); // max 1-byte without high bit
  checkGetCompact("ff", 0x0200ff00, __FILE__, __LINE__); // needs extra byte due to high bit
  checkGetCompact("80", 0x02008000, __FILE__, __LINE__); // 0x00800000 bit handling
  checkGetCompact("100", 0x02010000, __FILE__, __LINE__);
  checkGetCompact("7fff", 0x027fff00, __FILE__, __LINE__); // max 2-byte without high bit
  checkGetCompact("ffff", 0x0300ffff, __FILE__, __LINE__); // needs extra byte due to high bit
  checkGetCompact("123456", 0x03123456, __FILE__, __LINE__);
  checkGetCompact("12345600", 0x04123456, __FILE__, __LINE__); // rounds down

  // The sign is only applied to a nonzero mantissa
  if (acc && uint256GetCompact(UInt<256>::fromHex("123456"), true) != 0x03923456u) {
    fprintf(stderr, " * %s:%u negative getCompact mismatch\n", __FILE__, __LINE__);
    acc = false;
  }
  if (acc && uint256GetCompact(UInt<256>::zero(), true) != 0) {
    fprintf(stderr, " * %s:%u negative getCompact of zero mismatch\n", __FILE__, __LINE__);
    acc = false;
  }

  return acc;
}

bool testUintMulDouble()
{
  bool acc = true;

  auto check = [&acc](const char *hex, double multiplier, const char *expectedHex, const char *file, unsigned line) {
    if (!acc) return;

    UInt<256> v = UInt<256>::fromHex(hex);
    v.mulfp(multiplier);
    if (v.getHex(false) != expectedHex) {
      fprintf(stderr, " * %s:%u mulDouble mismatch for 0x%s * %.17g: got %s, expected %s\n",
              file, line, hex, multiplier, v.getHex(false).c_str(), expectedHex);
      acc = false;
    }
  };

  // Multiply by 1.0 (identity)
  check("1", 1.0, "1", __FILE__, __LINE__);
  check("ffffffff", 1.0, "ffffffff", __FILE__, __LINE__);
  check("ffffffffffffffff", 1.0, "ffffffffffffffff", __FILE__, __LINE__);

  // Multiply by 2.0
  check("1", 2.0, "2", __FILE__, __LINE__);
  check("100", 2.0, "200", __FILE__, __LINE__);
  check("ffffffff", 2.0, "1fffffffe", __FILE__, __LINE__);

  // Multiply by 0.5
  check("2", 0.5, "1", __FILE__, __LINE__);
  check("100", 0.5, "80", __FILE__, __LINE__);
  check("ffffffff", 0.5, "7fffffff", __FILE__, __LINE__);

  // Multiply by 10.0
  check("1", 10.0, "a", __FILE__, __LINE__);
  check("a", 10.0, "64", __FILE__, __LINE__);
  check("64", 10.0, "3e8", __FILE__, __LINE__);

  // Multiply by 0.1
  check("a", 0.1, "1", __FILE__, __LINE__);
  check("64", 0.1, "a", __FILE__, __LINE__);
  check("3e8", 0.1, "64", __FILE__, __LINE__);

  // Multiply by powers of 2
  check("1", 4.0, "4", __FILE__, __LINE__);
  check("1", 8.0, "8", __FILE__, __LINE__);
  check("1", 16.0, "10", __FILE__, __LINE__);
  check("1", 256.0, "100", __FILE__, __LINE__);
  check("1", 65536.0, "10000", __FILE__, __LINE__);

  // Multiply by fractional powers of 2
  check("100", 0.25, "40", __FILE__, __LINE__);
  check("100", 0.125, "20", __FILE__, __LINE__);

  // Large values
  check("10000000000000000", 2.0, "20000000000000000", __FILE__, __LINE__);  // 2^64 * 2
  check("10000000000000000", 0.5, "8000000000000000", __FILE__, __LINE__);   // 2^64 * 0.5

  // Verify precision: multiply by number that fits in 53-bit mantissa
  // 123456789 * 123456789 = 15241578750190521
  check("75bcd15", 123456789.0, "3626229738a3b9", __FILE__, __LINE__);

  // Multiply by very small fraction (should truncate to near zero for small values)
  check("ff", 0.001, "0", __FILE__, __LINE__);

  // Multiply by large number
  check("1", 4294967296.0, "100000000", __FILE__, __LINE__);  // 1 * 2^32

  // Multi-limb multiplication
  check("ffffffffffffffff", 2.0, "1fffffffffffffffe", __FILE__, __LINE__);
  check("ffffffffffffffffffffffffffffffff", 0.5, "7fffffffffffffffffffffffffffffff", __FILE__, __LINE__);

  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 0.0000000001, "6df37f675ef6ebffffffffffffffffffffffffffffffffffffffffff", __FILE__, __LINE__);


  // A multiplier above 2^53: the shift after the multiply goes left, not right
  check("1", 1e20, "56bc75e2d63100000", __FILE__, __LINE__);
  check("123456789abcdef0", 1e20, "62afbde1181c716a35000634bf000000", __FILE__, __LINE__);
  // the same, but the top limb is taken, so the work is done in a wider value
  check("1000000000000000000000000000000000000000000000003", 1e20,
        "6bc75e2d631000000000000000000000000000000000001043561a8829300000", __FILE__, __LINE__);

  // Exactly 2^52: no shift after the multiply at all
  check("3", 4503599627370496.0, "30000000000000", __FILE__, __LINE__);
  check("1000000000000000000000000000000000000000000000003", 4503599627370496.0,
        "10000000000000000000000000000000000000000000000030000000000000", __FILE__, __LINE__);

  return acc;
}
bool testUintFromDouble()
{
  bool acc = true;

  auto check = [&acc](double d, const char *expectedHex, const char *file, unsigned line) {
    if (!acc) return;

    UInt<256> v = UInt<256>::fromDouble(d);
    if (v.getHex(false) != expectedHex) {
      fprintf(stderr, " * %s:%u fromDouble mismatch for %.17g: got %s, expected %s\n",
              file, line, d, v.getHex(false).c_str(), expectedHex);
      acc = false;
    }
  };

  // Zero
  check(0.0, "0", __FILE__, __LINE__);

  // Small integers
  check(1.0, "1", __FILE__, __LINE__);
  check(2.0, "2", __FILE__, __LINE__);
  check(10.0, "a", __FILE__, __LINE__);
  check(100.0, "64", __FILE__, __LINE__);
  check(255.0, "ff", __FILE__, __LINE__);
  check(256.0, "100", __FILE__, __LINE__);
  check(1000.0, "3e8", __FILE__, __LINE__);

  // Powers of 2
  check(4294967296.0, "100000000", __FILE__, __LINE__);  // 2^32
  check(18446744073709551616.0, "10000000000000000", __FILE__, __LINE__);  // 2^64

  // Max uint32_t
  check(4294967295.0, "ffffffff", __FILE__, __LINE__);

  // Large powers of 2
  check(std::pow(2.0, 128), "100000000000000000000000000000000", __FILE__, __LINE__);
  check(std::pow(2.0, 192), "1000000000000000000000000000000000000000000000000", __FILE__, __LINE__);

  // Roundtrip test: UInt -> double -> UInt should preserve value for values that fit in double precision
  // Double has 53-bit mantissa, so use values up to 53 bits
  {
    UInt<256> original = UInt<256>::fromHex("1fffffffffffff");  // 53 bits, max exact double
    double d = original.getDouble();
    UInt<256> restored = UInt<256>::fromDouble(d);
    if (original != restored) {
      fprintf(stderr, " * %s:%u roundtrip failed for %s, got %s\n", __FILE__, __LINE__,
              original.getHex(false).c_str(), restored.getHex(false).c_str());
      acc = false;
    }
  }

  // Another roundtrip with smaller value
  {
    UInt<256> original = UInt<256>::fromHex("deadbeef");
    double d = original.getDouble();
    UInt<256> restored = UInt<256>::fromDouble(d);
    if (original != restored) {
      fprintf(stderr, " * %s:%u roundtrip failed for %s, got %s\n", __FILE__, __LINE__,
              original.getHex(false).c_str(), restored.getHex(false).c_str());
      acc = false;
    }
  }

  // Roundtrip for larger values (will lose precision but should be close)
  {
    UInt<256> original = UInt<256>::fromHex("ffffffffffffffffffff");  // 80 bits
    double d = original.getDouble();
    UInt<256> restored = UInt<256>::fromDouble(d);
    // Check relative error is small
    UInt<256> diff = (original > restored) ? (original - restored) : (restored - original);
    // Allow up to 2^27 difference for 80-bit value (53-bit mantissa means ~27 bits lost)
    UInt<256> tolerance = UInt<256>::fromHex("8000000");  // 2^27
    if (diff > tolerance) {
      fprintf(stderr, " * %s:%u large roundtrip error: original=%s, restored=%s\n",
              __FILE__, __LINE__, original.getHex(false).c_str(), restored.getHex(false).c_str());
      acc = false;
    }
  }

  // Fractional values should truncate
  check(1.5, "1", __FILE__, __LINE__);
  check(1.9, "1", __FILE__, __LINE__);
  check(99.99, "63", __FILE__, __LINE__);

  return acc;
}

bool testUintGetDecimal()
{
  bool acc = true;

  auto check = [&acc](const char *hex, const char *expectedDec, const char *file, unsigned line) {
    if (!acc) return;

    UInt<256> v = UInt<256>::fromHex(hex);
    std::string result = v.getDecimal();

    if (result != expectedDec) {
      fprintf(stderr, " * %s:%u getDecimal mismatch for 0x%s: got %s, expected %s\n",
              file, line, hex, result.c_str(), expectedDec);
      acc = false;
    }
  };

  // Zero
  check("0", "0", __FILE__, __LINE__);

  // Small values (single digit)
  check("1", "1", __FILE__, __LINE__);
  check("2", "2", __FILE__, __LINE__);
  check("9", "9", __FILE__, __LINE__);
  check("a", "10", __FILE__, __LINE__);
  check("f", "15", __FILE__, __LINE__);

  // Two-digit decimal
  check("10", "16", __FILE__, __LINE__);
  check("63", "99", __FILE__, __LINE__);
  check("64", "100", __FILE__, __LINE__);

  // Powers of 10
  check("a", "10", __FILE__, __LINE__);
  check("64", "100", __FILE__, __LINE__);
  check("3e8", "1000", __FILE__, __LINE__);
  check("2710", "10000", __FILE__, __LINE__);
  check("186a0", "100000", __FILE__, __LINE__);
  check("f4240", "1000000", __FILE__, __LINE__);

  // Powers of 2
  check("100", "256", __FILE__, __LINE__);
  check("10000", "65536", __FILE__, __LINE__);
  check("100000000", "4294967296", __FILE__, __LINE__);  // 2^32
  check("10000000000000000", "18446744073709551616", __FILE__, __LINE__);  // 2^64

  // Max uint64_t
  check("ffffffffffffffff", "18446744073709551615", __FILE__, __LINE__);

  // Known values
  check("ff", "255", __FILE__, __LINE__);
  check("ffff", "65535", __FILE__, __LINE__);
  check("ffffffff", "4294967295", __FILE__, __LINE__);  // max uint32_t
  check("deadbeef", "3735928559", __FILE__, __LINE__);
  check("cafebabe", "3405691582", __FILE__, __LINE__);

  // Multi-limb values
  check("ffffffffffffffffffffffffffffffff", "340282366920938463463374607431768211455", __FILE__, __LINE__);  // 2^128 - 1

  // Large specific values
  check("123456789abcdef0", "1311768467463790320", __FILE__, __LINE__);
  check("fedcba9876543210", "18364758544493064720", __FILE__, __LINE__);

  // 2^128
  check("100000000000000000000000000000000", "340282366920938463463374607431768211456", __FILE__, __LINE__);

  // 2^192
  check("1000000000000000000000000000000000000000000000000",
        "6277101735386680763835789423207666416102355444464034512896", __FILE__, __LINE__);

  // 2^256 - 1 (max 256-bit value)
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "115792089237316195423570985008687907853269984665640564039457584007913129639935", __FILE__, __LINE__);

  // Verify divmod10 doesn't corrupt the original for various patterns
  {
    const char* testCases[] = {
      "1",
      "a",
      "ff",
      "100",
      "ffffffff",
      "ffffffffffffffff",
      "10000000000000000",
      "ffffffffffffffffffffffffffffffff",
      "deadbeefcafebabe",
      "123456789abcdef0123456789abcdef0",
    };

    for (const char* hex : testCases) {
      UInt<256> original = UInt<256>::fromHex(hex);
      std::string dec = original.getDecimal();

      // Verify by converting back through multiplication
      // Simple check: the decimal string should be non-empty
      if (dec.empty() || (dec[0] == '0' && dec.length() > 1)) {
        fprintf(stderr, " * %s:%u invalid decimal output for 0x%s: got '%s'\n",
                __FILE__, __LINE__, hex, dec.c_str());
        acc = false;
      }
    }
  }

  // Verify consistency: getDecimal of 0 through 100
  for (uint64_t i = 0; i <= 100; i++) {
    UInt<256> v(i);
    std::string result = v.getDecimal();
    std::string expected = std::to_string(i);
    if (result != expected) {
      fprintf(stderr, " * %s:%u getDecimal mismatch for %llu: got %s, expected %s\n",
              __FILE__, __LINE__, (unsigned long long)i, result.c_str(), expected.c_str());
      acc = false;
      break;
    }
  }

  // Verify consistency for larger values where we can use standard library
  {
    UInt<256> v = UInt<256>::fromHex("ffffffffffffffff");
    std::string result = v.getDecimal();
    // 2^64 - 1 = 18446744073709551615
    if (result != "18446744073709551615") {
      fprintf(stderr, " * %s:%u max uint64 mismatch: got %s\n", __FILE__, __LINE__, result.c_str());
      acc = false;
    }
  }

  // Edge case: value with many trailing zeros in binary
  check("80000000000000000000000000000000",
        "170141183460469231731687303715884105728", __FILE__, __LINE__);  // 2^127

  return acc;
}

bool testUintFpdiv()
{
  bool acc = true;

  auto check = [&acc](const char *dividendHex, const char *divisorHex, double expected,
                      double tolerance, const char *file, unsigned line) {
    if (!acc) return;

    UInt<256> dividend = UInt<256>::fromHex(dividendHex);
    UInt<256> divisor = UInt<256>::fromHex(divisorHex);
    double result = UInt<256>::fpdiv(dividend, divisor);

    double diff = result - expected;
    if (diff < 0) diff = -diff;
    double tol = expected * tolerance;
    if (tol < 1e-15) tol = 1e-15;

    if (diff > tol) {
      fprintf(stderr, " * %s:%u fpdiv mismatch for 0x%s / 0x%s: got %.17g, expected %.17g\n",
              file, line, dividendHex, divisorHex, result, expected);
      acc = false;
    }
  };

  // Zero cases
  check("0", "1", 0.0, 1e-15, __FILE__, __LINE__);
  check("0", "ffffffff", 0.0, 1e-15, __FILE__, __LINE__);
  check("1", "0", 0.0, 1e-15, __FILE__, __LINE__);  // division by zero returns 0

  // Identity: n / 1 = n
  check("1", "1", 1.0, 1e-15, __FILE__, __LINE__);
  check("a", "1", 10.0, 1e-15, __FILE__, __LINE__);
  check("64", "1", 100.0, 1e-15, __FILE__, __LINE__);
  check("ffffffff", "1", 4294967295.0, 1e-15, __FILE__, __LINE__);

  // Simple divisions
  check("a", "2", 5.0, 1e-15, __FILE__, __LINE__);        // 10 / 2 = 5
  check("64", "a", 10.0, 1e-15, __FILE__, __LINE__);      // 100 / 10 = 10
  check("3e8", "a", 100.0, 1e-15, __FILE__, __LINE__);    // 1000 / 10 = 100

  // Divisions with remainder
  check("a", "3", 3.3333333333333333, 1e-14, __FILE__, __LINE__);   // 10 / 3
  check("7", "3", 2.3333333333333333, 1e-14, __FILE__, __LINE__);   // 7 / 3
  check("1", "3", 0.3333333333333333, 1e-14, __FILE__, __LINE__);   // 1 / 3

  // Fractions < 1
  check("1", "2", 0.5, 1e-15, __FILE__, __LINE__);
  check("1", "4", 0.25, 1e-15, __FILE__, __LINE__);
  check("1", "a", 0.1, 1e-15, __FILE__, __LINE__);
  check("1", "64", 0.01, 1e-15, __FILE__, __LINE__);

  // Powers of 2
  check("100", "10", 16.0, 1e-15, __FILE__, __LINE__);              // 256 / 16
  check("10000000000000000", "100000000", 4294967296.0, 1e-15, __FILE__, __LINE__);  // 2^64 / 2^32

  // Large numbers (multi-limb)
  check("ffffffffffffffff", "1", 18446744073709551615.0, 1e-15, __FILE__, __LINE__);  // max uint64
  check("ffffffffffffffff", "ffffffffffffffff", 1.0, 1e-15, __FILE__, __LINE__);      // equal
  check("fffffffffffffffe", "ffffffffffffffff", 0.9999999999999999, 1e-14, __FILE__, __LINE__);

  // 2^128 / 2^64 = 2^64
  check("100000000000000000000000000000000", "10000000000000000",
        18446744073709551616.0, 1e-15, __FILE__, __LINE__);

  // 2^128 / 2^127 = 2
  check("100000000000000000000000000000000", "80000000000000000000000000000000",
        2.0, 1e-15, __FILE__, __LINE__);

  // Very large: 2^256-1 / 2^128 ≈ 2^128
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "100000000000000000000000000000000",
        340282366920938463463374607431768211455.0, 1e-10, __FILE__, __LINE__);

  // 2^200 / 2^100 = 2^100
  check("100000000000000000000000000000000000000000000000000",
        "10000000000000000000000000",
        1267650600228229401496703205376.0, 1e-15, __FILE__, __LINE__);

  // Random-ish large numbers
  check("deadbeefcafebabedeadbeefcafebabe",
        "deadbeefcafebabe",
        18446744073709551617.0, 1e-10, __FILE__, __LINE__);

  // Very small result from large numbers
  check("1", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        8.636168555094444e-78, 1e-10, __FILE__, __LINE__);

  // Verify consistency: a / b * b ≈ a for various values
  {
    const char* testCases[][2] = {
      {"123456789abcdef0", "fedcba9876543210"},
      {"ffffffffffffffff", "2"},
      {"100000000000000000000000000000000", "3"},
      {"deadbeef", "cafebabe"},
    };

    for (const auto& tc : testCases) {
      UInt<256> a = UInt<256>::fromHex(tc[0]);
      UInt<256> b = UInt<256>::fromHex(tc[1]);
      double ratio = UInt<256>::fpdiv(a, b);
      double restored = ratio * b.getDouble();
      double original = a.getDouble();

      double relError = std::abs(restored - original) / original;
      if (relError > 1e-10) {
        fprintf(stderr, " * %s:%u fpdiv consistency failed for 0x%s / 0x%s: relError=%.2e\n",
                __FILE__, __LINE__, tc[0], tc[1], relError);
        acc = false;
      }
    }
  }

  return acc;
}

bool testUintGetDouble()
{
  bool acc = true;

  auto check = [&acc](const char *hex, double expected, const char *file, unsigned line) {
    if (!acc) return;

    UInt<256> v = UInt<256>::fromHex(hex);
    double result = v.getDouble();

    // Compare with relative tolerance for floating point
    double diff = result - expected;
    if (diff < 0) diff = -diff;
    double tolerance = expected * 1e-15;
    if (tolerance < 1e-15) tolerance = 1e-15;

    if (diff > tolerance) {
      fprintf(stderr, " * %s:%u getDouble mismatch for %s: got %.17g, expected %.17g\n",
              file, line, hex, result, expected);
      acc = false;
    }
  };

  // Zero
  check("0", 0.0, __FILE__, __LINE__);

  // Small values (fit in single limb)
  check("1", 1.0, __FILE__, __LINE__);
  check("2", 2.0, __FILE__, __LINE__);
  check("a", 10.0, __FILE__, __LINE__);
  check("64", 100.0, __FILE__, __LINE__);
  check("ff", 255.0, __FILE__, __LINE__);
  check("100", 256.0, __FILE__, __LINE__);
  check("ffffffff", 4294967295.0, __FILE__, __LINE__);
  check("ffffffffffffffff", 18446744073709551615.0, __FILE__, __LINE__);

  // Two-limb values
  check("10000000000000000", 18446744073709551616.0, __FILE__, __LINE__);  // 2^64
  check("20000000000000000", 2.0 * 18446744073709551616.0, __FILE__, __LINE__);  // 2^65

  // Powers of 2
  check("100000000000000000000000000000000", 340282366920938463463374607431768211456.0, __FILE__, __LINE__);  // 2^128

  // Large values (may lose precision but should be approximately correct)
  check("ffffffffffffffffffffffffffffffff",
        340282366920938463463374607431768211455.0, __FILE__, __LINE__);  // 2^128 - 1

  // Full 256-bit value (maximum)
  // 2^256 - 1 ≈ 1.1579208923731619542357098500869e+77
  check("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        115792089237316195423570985008687907853269984665640564039457584007913129639935.0,
        __FILE__, __LINE__);

  return acc;
}
