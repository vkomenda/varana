#pragma once

#include <ap_int.h>
#include <cstdint>
#include <tapa.h>

// Number of hash kernels.
constexpr int NK = 16;

ap_uint<256> sha256(ap_uint<256> msg);

ap_uint<256> reverse_bytes_u256(ap_uint<256> n);

void poh(tapa::mmap<ap_uint<256>> in_hashes,
         tapa::mmap<uint64_t> num_iters,
         uint32_t num_hashes,
         tapa::mmap<ap_uint<256>> out_hashes);
