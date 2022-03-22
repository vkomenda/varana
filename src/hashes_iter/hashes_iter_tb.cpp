#include "hashes_iter.h"

#define NUM_PKTS 128
#define IN_PKTS_SIZE (NUM_PKTS * IN_PKT_SIZE)
#define NUM_WORDS (IN_PKTS_SIZE / XDMA_AXIS_WIDTH + (IN_PKTS_SIZE % XDMA_AXIS_WIDTH ? 1 : 0))
#define NUM_ITERS 8

int mux_in_pkts(ap_uint<IN_PKT_SIZE> in_pkts[NUM_PKTS], hls::stream<xdma_axis_t> &in_words) {
    int pos = XDMA_AXIS_WIDTH - 1;
    xdma_axis_t word;
    word.data = 0;
    unsigned word_count = 0;
    for (unsigned i = 0; i < NUM_PKTS; i++) {
        if (pos >= XDMA_AXIS_WIDTH) {
            return 1;
        }
        if (pos < 0) {
            return 1;
        }

        if (pos >= IN_PKT_SIZE - 1) {
            if (i == NUM_PKTS - 1) {
                word.last = true;
            }
            word.data(pos, pos - IN_PKT_SIZE + 1) = in_pkts[i];

            pos -= IN_PKT_SIZE;
            if (pos < -1) {
                return 1;
            } else if (pos == -1) {
                // Start a new word with the next packet.
                in_words.write(word);
                word.data = 0;
                word_count++;
                pos = XDMA_AXIS_WIDTH - 1;
            }
        } else {
            // Finish the current word.
            word.data(pos, 0) = in_pkts[i](IN_PKT_SIZE - 1, IN_PKT_SIZE - 1 - pos);

            in_words.write(word);
            word.data = 0;
            word_count++;

            word.data(XDMA_AXIS_WIDTH - 1, XDMA_AXIS_WIDTH - IN_PKT_SIZE + pos + 1) =
                in_pkts[i](IN_PKT_SIZE - pos - 2, 0);

            // Write out only if this is the last word.
            if (i == NUM_PKTS - 1) {
                word.last = true;
                in_words.write(word);
                word_count++;
            }
            else {
                pos = XDMA_AXIS_WIDTH - IN_PKT_SIZE + pos;
            }
        }
    }
    if (word_count != NUM_WORDS) {
        return 1;
    }

    return 0;
}

int test_mux_in_pkts() {
    hls::stream<xdma_axis_t> in_words;
    ap_uint<IN_PKT_SIZE> in_pkts[NUM_PKTS];

    // Initialize in_pkts.
    for (unsigned i = 0; i < NUM_PKTS; i++) {
        in_pkts[i](IN_PKT_SIZE - 1, NUM_ITERS_SIZE) = i + 1;
        in_pkts[i](NUM_ITERS_SIZE - 1, 0) = NUM_ITERS;
        std::cout << in_pkts[i].to_string(16, true).c_str() << std::endl;
    }

    mux_in_pkts(in_pkts, in_words);

    xdma_axis_t word1 = in_words.read();
    xdma_axis_t word2 = in_words.read();

    // EXPECT_PRED_FORMAT2(AssertEqHex,
    //                     word1.data(XDMA_AXIS_WIDTH - 1, XDMA_AXIS_WIDTH - HASH_SIZE),
    //                     1);
    // EXPECT_PRED_FORMAT2(AssertEqHex,
    //                     word1.data(XDMA_AXIS_WIDTH - HASH_SIZE - 1, XDMA_AXIS_WIDTH - IN_PKT_SIZE),
    //                     NUM_ITERS);
    // EXPECT_PRED_FORMAT2(AssertEqHex,
    //                     word1.data(XDMA_AXIS_WIDTH - IN_PKT_SIZE - 1, 0),
    //                     0);

    // EXPECT_PRED_FORMAT2(AssertEqHex,
    //                     word2.data(XDMA_AXIS_WIDTH - 1,
    //                                (2 * XDMA_AXIS_WIDTH - IN_PKT_SIZE - HASH_SIZE) % XDMA_AXIS_WIDTH),
    //                     2);
    // EXPECT_PRED_FORMAT2(AssertEqHex,
    //                     word2.data((2 * XDMA_AXIS_WIDTH - IN_PKT_SIZE - HASH_SIZE) % XDMA_AXIS_WIDTH - 1,
    //                                (2 * XDMA_AXIS_WIDTH - 2 * IN_PKT_SIZE) % XDMA_AXIS_WIDTH),
    //                     NUM_ITERS);

    return 0;
}

int test_hashes_iter() {
    hls::stream<xdma_axis_t> in_words, out_words;
    ap_uint<IN_PKT_SIZE> in_pkts[NUM_PKTS];
    ap_uint<256> gmem[NUM_PKTS];
    unsigned i;

    // Initialize in_pkts.
    for (i = 0; i < NUM_PKTS; i++) {
        in_pkts[i](IN_PKT_SIZE - 1, NUM_ITERS_SIZE) = i + 1;
        in_pkts[i](NUM_ITERS_SIZE - 1, 0) = i;
        gmem[i] = 0;
        // std::cout << in_pkts[i].to_string(16, true).c_str() << std::endl;
    }

    // std::cout << "Task fifo ready? " << is_task_fifo_ready() << std::endl;

    int ret = mux_in_pkts(in_pkts, in_words);
    if (ret) return ret;

    hashes_iter(in_words, out_words, gmem);

    for (unsigned i = 0; i < NUM_PKTS; i += 2) {
        ap_uint<256> start_hash1 = ap_uint<256>(i + 1);
        ap_uint<256> start_hash2 = ap_uint<256>(i + 2);
        ap_uint<256> expected_hash1 = start_hash1;
        ap_uint<256> expected_hash2 = start_hash2;
        for (unsigned j = 0; j < i; j++) {
            expected_hash1 = expected_hash1 + expected_hash1;
            expected_hash2 = expected_hash2 + expected_hash2;
        }
        expected_hash2 = expected_hash2 + expected_hash2;

        xdma_axis_t word = out_words.read();
        ap_uint<256> hash1 = word.data(511, 256);
        ap_uint<256> hash2 = word.data(255, 0);

        if (hash1 != expected_hash1) {
            return 1;
        }
        if (hash2 != expected_hash2) {
            return 1;
        }
    }

    // hls::stream<xdma_axis_t> in_words, out_words;
    // ap_uint<IN_PKT_SIZE> in_pkts[NUM_PKTS];

    // for (unsigned pass = 0; pass < 2; pass++) {
    //     // Initialize in_pkts.
    //     for (unsigned i = 0; i < NUM_PKTS / 2; i++) {
    //         in_pkts[i](IN_PKT_SIZE - 1, NUM_ITERS_SIZE) = 2 * i + 1;
    //         in_pkts[i](NUM_ITERS_SIZE - 1, 0) = i;
    //         std::cout << in_pkts[i].to_string(16, true).c_str() << std::endl;
    //     }
    //     std::cout << "Task fifo ready? " << is_task_fifo_ready() << std::endl;

    //     mux_in_pkts(in_pkts, in_words);
    //     hashes_iter(in_words, out_words);

    //     for (unsigned i = 0; i < NUM_PKTS / 2; i++) {
    //         xdma_axis_t word = out_words.read();
    //         ap_uint<256> start_hash, computed_hash;
    //         start_hash = word.data(511, 256);
    //         computed_hash = word.data(255, 0);
    //         // std::cout << start_hash << ", " << computed_hash << std::endl;
    //         EXPECT_PRED_FORMAT2(AssertEqHex, start_hash, ap_uint<256>(2 * i + 1));
    //         ap_uint<256> expected_hash = start_hash;
    //         for (unsigned j = 0; j < i; j++) {
    //             expected_hash = expected_hash + expected_hash;
    //         }
    //         EXPECT_PRED_FORMAT2(AssertEqHex, computed_hash, expected_hash);
    //     }
    // }
    // std::cout << "Famous last word " << in_words.read().data.to_string(16, true).c_str() << std::endl;

    // unsigned task_id = take_task_fifo_head();
    // unsigned expected_task_id = 0;
    // while (task_id >= 0 && expected_task_id < IN_PKT_PAR) {
    //     if (task_id != expected_task_id++) {
    //         return 1;
    //     }
    //     task_id = take_task_fifo_head();
    // }
    // if (task_id != -1) {
    //     return -1;
    // }

    return 0;
}

int main() {
    int ret;

    ret = test_hashes_iter();
    if (ret) {
        return ret;
    }

    return 0;
}
