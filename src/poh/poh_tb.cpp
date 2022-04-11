#include "poh.h"

#define NUM_HASHES (BATCH_NUM_HASHES + 2)

// Byte-reversed hashes simulating the big-endian storage. Note that ap_uint<256> is little-endian.
const ap_uint<256> IN_HASH =
    ap_uint<256>("6b54cada05988fef58c0094e96ceee4eb62451c0a791b011e96f0bc81947ba01", 16);
const ap_uint<256> OUT_HASH =
    ap_uint<256>("7adde1c45648b90ab9afca114d60b584ff599cc46b70852fb41940b90172829c", 16);

int test_poh() {
    ap_uint<256> in_hashes[NUM_HASHES], out_hashes[NUM_HASHES];
    ap_uint<64> num_iters[NUM_HASHES];

    ap_uint<256> expected_hash = sha256(reverse_bytes_u256(IN_HASH));
    if (expected_hash != reverse_bytes_u256(OUT_HASH)) {
        std::cout << "sha256 error" << std::endl;
        return 1;
    }

    // Initialize inputs.
    for (unsigned i = 0; i < NUM_HASHES; i++) {
        in_hashes[i] = reverse_bytes_u256(IN_HASH);
        num_iters[i] = ap_uint<64>(1);
    }

    unsigned num_hashes = NUM_HASHES;
    std::cout << "Starting the core..." << std::endl;
    poh(in_hashes, num_iters, num_hashes, out_hashes);

    for (unsigned i = 0; i < NUM_HASHES; i++) {
        ap_uint<256> out_hash = out_hashes[i];
        std::cout << i << ". got      " << out_hash.to_string(16, true).c_str() << std::endl;
        std::cout << i << ". expected " << expected_hash.to_string(16, true).c_str() << std::endl;
        if (out_hash != reverse_bytes_u256(OUT_HASH)) {
            return 1;
        }
    }
    std::cout << "Success!" << std::endl;
    return 0;
}

int main() {
    int ret;

    ret = test_poh();
    if (ret) {
        return ret;
    }

    return 0;
}
