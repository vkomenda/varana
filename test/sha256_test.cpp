#include <ap_int.h>
#include <gtest/gtest.h>
#include <sha256.h>

#define TEST_VECTOR_256 ap_uint<256>("0x5f48828a7ecafc69180e8e9607b277b3dd17e53acc043412a06d8f170711c74b", 16)
#define EXPECTED_SHA256 ap_uint<256>("0x2a7b86127b7a7f53fa9a79d0e7d01cc479beae8a579c9f5ba95c1bbb68784ec6", 16)

testing::AssertionResult AssertEqHex(const char* m_expr,
                                     const char* n_expr,
                                     ap_uint<256> m,
                                     ap_uint<256> n) {
  if (m == n) return testing::AssertionSuccess();

  return testing::AssertionFailure() << m_expr << " != " << n_expr << std::endl
                                     << m.to_string(16, true).c_str() << " != "
                                     << n.to_string(16, true).c_str();
}

TEST(Sha256, UnitTest) {
    ap_uint<256> msg = TEST_VECTOR_256;
    ap_uint<256> hash = sha256(msg);
    EXPECT_PRED_FORMAT2(AssertEqHex, hash, EXPECTED_SHA256);
}
