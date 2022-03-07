#pragma once

#include <ap_int.h>
#include <gtest/gtest.h>

testing::AssertionResult AssertEqHex(const char* m_expr,
                                     const char* n_expr,
                                     ap_uint<256> m,
                                     ap_uint<256> n) {
  if (m == n) return testing::AssertionSuccess();

  return testing::AssertionFailure() << m_expr << " != " << n_expr << std::endl
                                     << m.to_string(16, true).c_str() << " != "
                                     << n.to_string(16, true).c_str();
}
