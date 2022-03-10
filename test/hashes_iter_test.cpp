#include <gtest/gtest.h>
#include <iostream>
#include "hashes_iter.h"
#include "ap_assert.h"

#define NUM_PKTS IN_PKT_PAR  // TODO: remove the dependency on IN_PKT_PAR
#define IN_PKTS_SIZE (NUM_PKTS * IN_PKT_SIZE)
#define NUM_WORDS (IN_PKTS_SIZE / XDMA_AXIS_WIDTH + (IN_PKTS_SIZE % XDMA_AXIS_WIDTH ? 1 : 0))
#define NUM_ITERS 8

void mux_in_pkts(ap_uint<IN_PKT_SIZE> in_pkts[NUM_PKTS], hls::stream<xdma_axis_t> &in_words) {
    int pos = XDMA_AXIS_WIDTH - 1;
    xdma_axis_t word;
    word.data = 0;
    unsigned word_count = 0;
    for (unsigned i = 0; i < NUM_PKTS; i++) {
        ASSERT_LT(pos, XDMA_AXIS_WIDTH);
        ASSERT_GE(pos, 0);

        if (pos >= IN_PKT_SIZE - 1) {
            word.data(pos, pos - IN_PKT_SIZE + 1) = in_pkts[i];

            pos -= IN_PKT_SIZE;
            ASSERT_GE(pos, -1);
            if (pos == -1) {
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
                in_words.write(word);
                word_count++;
            }
            else {
                pos = XDMA_AXIS_WIDTH - IN_PKT_SIZE + pos;
            }
        }
    }
    ASSERT_EQ(word_count, NUM_WORDS);
}

TEST(HashesIter, MuxInPkts) {
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

    EXPECT_PRED_FORMAT2(AssertEqHex,
                        word1.data(XDMA_AXIS_WIDTH - 1, XDMA_AXIS_WIDTH - HASH_SIZE),
                        1);
    EXPECT_PRED_FORMAT2(AssertEqHex,
                        word1.data(XDMA_AXIS_WIDTH - HASH_SIZE - 1, XDMA_AXIS_WIDTH - IN_PKT_SIZE),
                        NUM_ITERS);
    EXPECT_PRED_FORMAT2(AssertEqHex,
                        word1.data(XDMA_AXIS_WIDTH - IN_PKT_SIZE - 1, 0),
                        0);

    EXPECT_PRED_FORMAT2(AssertEqHex,
                        word2.data(XDMA_AXIS_WIDTH - 1,
                                   (2 * XDMA_AXIS_WIDTH - IN_PKT_SIZE - HASH_SIZE) % XDMA_AXIS_WIDTH),
                        2);
    EXPECT_PRED_FORMAT2(AssertEqHex,
                        word2.data((2 * XDMA_AXIS_WIDTH - IN_PKT_SIZE - HASH_SIZE) % XDMA_AXIS_WIDTH - 1,
                                   (2 * XDMA_AXIS_WIDTH - 2 * IN_PKT_SIZE) % XDMA_AXIS_WIDTH),
                        NUM_ITERS);
}

TEST(HashesIter, TestPacketsComplete) {
    hls::stream<xdma_axis_t> in_words, out_words;
    ap_uint<IN_PKT_SIZE> in_pkts[NUM_PKTS];

    // Initialize in_pkts.
    for (unsigned i = 0; i < NUM_PKTS; i++) {
        in_pkts[i](IN_PKT_SIZE - 1, NUM_ITERS_SIZE) = i + 1;
        in_pkts[i](NUM_ITERS_SIZE - 1, 0) = i;
        std::cout << in_pkts[i].to_string(16, true).c_str() << std::endl;
    }

    std::cout << "Task fifo ready? " << is_task_fifo_ready() << std::endl;

    mux_in_pkts(in_pkts, in_words);
    hashes_iter(in_words, out_words);

    for (unsigned i = 0; i < NUM_PKTS; i++) {
        xdma_axis_t word = out_words.read();
        ap_uint<256> start_hash, computed_hash;
        start_hash = word.data(511, 256);
        computed_hash = word.data(255, 0);
        // std::cout << start_hash << ", " << computed_hash << std::endl;
        EXPECT_PRED_FORMAT2(AssertEqHex, start_hash, ap_uint<256>(i + 1));
        ap_uint<256> expected_hash = start_hash;
        for (unsigned j = 0; j < i; j++) {
            expected_hash = expected_hash + expected_hash;
        }
        EXPECT_PRED_FORMAT2(AssertEqHex, computed_hash, expected_hash);
    }
    // std::cout << "Famous last word " << in_words.read().data.to_string(16, true).c_str() << std::endl;
}

TEST(HashesIter, TestPacketsFourHalfStreams) {
    hls::stream<xdma_axis_t> in_words, out_words;
    ap_uint<IN_PKT_SIZE> in_pkts[NUM_PKTS];


    for (unsigned pass = 0; pass < 2; pass++) {
        // Initialize in_pkts.
        for (unsigned i = 0; i < NUM_PKTS / 2; i++) {
            in_pkts[i](IN_PKT_SIZE - 1, NUM_ITERS_SIZE) = i + 1;
            in_pkts[i](NUM_ITERS_SIZE - 1, 0) = i;
            std::cout << in_pkts[i].to_string(16, true).c_str() << std::endl;
        }
        std::cout << "Task fifo ready? " << is_task_fifo_ready() << std::endl;

        mux_in_pkts(in_pkts, in_words);
        hashes_iter(in_words, out_words);
    }

    for (unsigned i = 0; i < NUM_PKTS; i++) {
        xdma_axis_t word = out_words.read();
        ap_uint<256> start_hash, computed_hash;
        start_hash = word.data(511, 256);
        computed_hash = word.data(255, 0);
        // std::cout << start_hash << ", " << computed_hash << std::endl;
        EXPECT_PRED_FORMAT2(AssertEqHex, start_hash, ap_uint<256>(i + 1));
        ap_uint<256> expected_hash = start_hash;
        for (unsigned j = 0; j < i; j++) {
            expected_hash = expected_hash + expected_hash;
        }
        EXPECT_PRED_FORMAT2(AssertEqHex, computed_hash, expected_hash);
    }
    // std::cout << "Famous last word " << in_words.read().data.to_string(16, true).c_str() << std::endl;
}
