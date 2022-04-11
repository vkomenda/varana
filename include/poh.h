#pragma once
#include <ap_int.h>
#include "sha256.h"

#define BATCH_NUM_HASHES 640   // Reads of input hashes and num_iters are aligned to the 4 KB boundary

ap_uint<256> reverse_bytes_u256(ap_uint<256> n);

void poh(const ap_uint<256> *in_hashes, // input hashes
         const ap_uint<64> *num_iters,  // input numbers of iterations for each input hash
         unsigned num_hashes,           // input number of hashes (the number of elements in in_hashes and in num_iters)
         ap_uint<256> *out_hashes       // output hashes
);
