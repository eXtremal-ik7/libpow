#include "test/sha256Test.h"
#include "test/sha3Test.h"
#include "test/uintTest.h"
#include "test/uintOpsTest.h"
#include <functional>
#include <string>
#include <stdio.h>

static unsigned numFailed = 0;

void runTest(const std::function<bool()> &testProc, const std::string &name)
{
  if (testProc()) {
    fprintf(stderr, "[OK] %s\n", name.c_str());
  } else {
    fprintf(stderr, "[FAILED] %s\n", name.c_str());
    numFailed++;
  }
}

int main(int, char **)
{
  runTest(testUintGetDouble, "UInt::GetDouble");
  runTest(testUintFromDouble, "UInt::FromDouble");
  runTest(testUintMulDouble, "UInt::MulDouble");
  runTest(testUintFpdiv, "UInt::Fpdiv");
  runTest(testUintShl, "UInt::Shl");
  runTest(testUintShr, "UInt::Shr");
  runTest(testUintAdd64, "UInt::Add64");
  runTest(testUintAdd, "UInt::Add");
  runTest(testUintSub64, "UInt::Sub64");
  runTest(testUintSub, "UInt::Sub");
  runTest(testUintMul64, "UInt::Mul64");
  runTest(testUintMul, "UInt::Mul");
  runTest(testUintDiv64, "UInt::Div64");
  runTest(testUintMod64, "UInt::Mod64");
  runTest(testUintDivmod, "UInt::DivMod");
  runTest(testUintCmp, "UInt::Cmp");
  runTest(testUintCmp64, "UInt::Cmp64");
  runTest(testUintCompact, "UInt::Compact");
  runTest(testUintGetDecimal, "UInt::GetDecimal");
  runTest(testUintBitwise, "UInt::Bitwise");
  runTest(testUintSar, "UInt::Sar");
  runTest(testUintByte, "UInt::Byte");
  runTest(testUintSignExtend, "UInt::SignExtend");
  runTest(testUintSignedCmp, "UInt::SignedCmp");
  runTest(testUintSdivmod, "UInt::Sdivmod");
  runTest(testUintAddmod, "UInt::Addmod");
  runTest(testUintMulmod, "UInt::Mulmod");
  runTest(testUintExp, "UInt::Exp");
  runTest(testUintWideProduct, "UInt::WideProduct");
  runTest(testUintBytes, "UInt::Bytes");
  runTest(testUintDivmodCorrections, "UInt::DivmodCorrections");
  runTest(testUintIdentities, "UInt::Identities");

  runTest(testSha256Nist, "Sha256::Nist");
  runTest(testSha256Lengths, "Sha256::Lengths");
  runTest(testSha256Streaming, "Sha256::Streaming");
  runTest(testSha256LowLevel, "Sha256::LowLevel");

  runTest(testSha3Nist, "Sha3::Nist");
  runTest(testSha3Keccak, "Sha3::Keccak");
  runTest(testSha3Lengths, "Sha3::Lengths");
  runTest(testSha3Streaming, "Sha3::Streaming");
  runTest(testSha3LowLevel, "Sha3::LowLevel");

  return numFailed != 0;
}
