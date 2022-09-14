#pragma once
#include <ap_int.h>
#include <cstdint>

ap_uint<256> sha256(ap_uint<256> msg, uint64_t n);
