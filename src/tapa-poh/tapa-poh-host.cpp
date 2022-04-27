#include <chrono>
#include <iostream>
#include <vector>

#include <ap_int.h>
#include <gflags/gflags.h>
#include <tapa.h>

using std::clog;
using std::endl;
using std::vector;
using std::chrono::duration;
using std::chrono::high_resolution_clock;

const uint32_t DEFAULT_NUM_HASHES = 128;
// Byte-reversed hashes simulating the big-endian storage. Note that ap_uint<256> is little-endian.
const ap_uint<256> IN_HASH =
    ap_uint<256>("6b54cada05988fef58c0094e96ceee4eb62451c0a791b011e96f0bc81947ba01", 16);
const ap_uint<256> OUT_HASH =
    ap_uint<256>("7adde1c45648b90ab9afca114d60b584ff599cc46b70852fb41940b90172829c", 16);

ap_uint<256> sha256(ap_uint<256> msg);

void poh(tapa::mmap<ap_uint<256>> in_hashes,
         tapa::mmap<uint64_t> num_iters,
         uint32_t num_hashes,
         tapa::mmap<ap_uint<256>> out_hashes);

DEFINE_string(bitstream, "", "path to bitstream file, run csim if empty");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    const uint32_t n = argc > 1 ? atoll(argv[1]) : DEFAULT_NUM_HASHES;

    vector<ap_uint<256>> hashes(n);
    vector<uint64_t> num_iters(n);
    vector<ap_uint<256>> results(n);

    for (uint32_t i = 0; i < n; i++) {
        hashes[i] = IN_HASH;
        num_iters[i] = i;
    }

    auto start = high_resolution_clock::now();

    tapa::invoke(poh, FLAGS_bitstream,
                 tapa::read_only_mmap<ap_uint<256>>(hashes),
                 tapa::read_only_mmap<uint64_t>(num_iters),
                 n,
                 tapa::write_only_mmap<ap_uint<256>>(results));

    auto stop = high_resolution_clock::now();
    duration<double> elapsed = stop - start;
    clog << "elapsed time: " << elapsed.count() << " s" << endl;

    uint32_t num_errors = 0;
    const uint32_t error_threshold = 10;

    for (uint32_t i = 0; i < n; i++) {
        auto result = results[i];
        ap_uint<256> expected;
        if (i == 0) {
            expected = IN_HASH;
        } else if (i == 1) {
            // Check that a single application of sha256 yields the correct result.
            expected = OUT_HASH;
        } else {
            expected = OUT_HASH;
            for (uint64_t j = 1; j < i; j++) {
                expected = sha256(expected);
            }
        }
        if (result != expected) {
            num_errors++;
            clog << "expected: " << expected << ", got: " << result << endl;
            if (num_errors >= error_threshold) {
                clog << "too many errors..." << endl;
                break;
            }
        }
    }
    if (num_errors == 0) {
        clog << "PASS" << endl;
    } else {
        clog << "FAIL" << endl;
    }

    return num_errors > 0 ? 1 : 0;
}
