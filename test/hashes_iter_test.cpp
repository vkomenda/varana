#include <gtest/gtest.h>
#include "hashes_iter.h"

#define NUM_PKTS 100
#define IN_PKTS_SIZE (NUM_PKTS * IN_PKT_SIZE)
#define NUM_WORDS (IN_PKTS_SIZE / XDMA_AXIS_WIDTH + (IN_PKTS_SIZE % XDMA_AXIS_WIDTH ? 1 : 0))
#define NUM_ITERS 8

TEST(HashesIter, Test100Packets) {
    hls::stream<xdma_axis_t> in_words, out_words;
    ap_uint<256> *gmem; // unused
    ap_uint<IN_PKT_SIZE> in_pkts[NUM_PKTS];

    // Initialize in_pkts.
    for (unsigned i = 0; i < NUM_PKTS; i++) {
        in_pkts[i](IN_PKT_SIZE - 1, NUM_ITERS_SIZE) = i;
        in_pkts[i](NUM_ITERS_SIZE - 1, 0) = NUM_ITERS;
    }

    // Starting position in the output word.
    int pos = XDMA_AXIS_WIDTH - 1;
    xdma_axis_t word;
    unsigned word_count = 0;
    for (unsigned i = 0; i < NUM_PKTS; i++) {
        ASSERT_LT(pos, XDMA_AXIS_WIDTH);
        ASSERT_GE(pos, 0);

        if (pos >= IN_PKT_SIZE - 1) {
            word.data(pos, pos - IN_PKT_SIZE + 1) = in_pkts[i];
            in_words.write(word);

            pos -= IN_PKT_SIZE;
            ASSERT_GE(pos, -1);
            if (pos == -1) {
                // Start a new word with the next packet.
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

    hashes_iter(in_words, out_words, gmem);

    pos = XDMA_AXIS_WIDTH - 1;
    for (unsigned i = 0; i < NUM_PKTS; i++) {
        xdma_axis_t word = out_words.read();
        ap_uint<256> start_hash, computed_hash;
        start_hash = word.data(511, 256);
        computed_hash = word.data(255, 0);
        ASSERT_EQ(start_hash, ap_uint<256>(i));
        ASSERT_EQ(computed_hash, ap_uint<256>(i * NUM_ITERS));
    }

    ASSERT_EQ(gmem[0], ap_uint<256>(0xc0ffee));
}
