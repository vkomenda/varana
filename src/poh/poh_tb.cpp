#include "poh.h"

#define NUM_HASHES (BATCH_NUM_HASHES / 2 + BATCH_NUM_HASHES)

int test_poh() {
    ap_uint<256> in_hashes[NUM_HASHES], out_hashes[NUM_HASHES];
    ap_uint<64> num_iters[NUM_HASHES];
    unsigned i;

    // Initialize inputs.
    for (i = 0; i < NUM_HASHES; i++) {
        in_hashes[i] = ap_uint<256>(1);
        num_iters[i] = ap_uint<64>(i);
    }

    unsigned num_hashes = NUM_HASHES;
    std::cout << "Starting the core..." << std::endl;
    poh(in_hashes, num_iters, num_hashes, out_hashes);

    for (unsigned i = 0; i < NUM_HASHES; i++) {
        ap_uint<256> start_hash = in_hashes[i];
        ap_uint<256> expected_hash = start_hash;
        for (unsigned j = 0; j < i; j++) {
            expected_hash = sha256(expected_hash);
        }

        std::cout << i << ". got      " << out_hashes[i].to_string(16, true).c_str() << std::endl;
        std::cout << i << ". expected " << expected_hash.to_string(16, true).c_str() << std::endl;
        if (out_hashes[i] != expected_hash) {
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
