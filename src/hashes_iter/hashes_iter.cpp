#include "ap_axi_sdata.h"
#include "hls_stream.h"

#define XDMA_AXIS_WIDTH 512
#define HASH_SIZE 256
#define NUM_ITERS_SIZE 64
#define IN_PKT_SIZE (HASH_SIZE + NUM_ITERS_SIZE)
// The number of parallel input packets in the processing pipeline. Its product with IN_PKT_SIZE must be
// evenly divisible by XDMA_AXIS_WIDTH.
#define IN_PKT_PAR 8
// Reserved packet designating the end of stream processing.
#define IN_PKTS_TERMINATOR (ap_int<IN_PKT_SIZE>("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 16))

typedef ap_axiu<XDMA_AXIS_WIDTH, 1, 1, 1> xdma_axis_t;
typedef ap_axiu<IN_PKT_SIZE, 1, 1, 1> in_pkt_t;
// typedef ap_axiu<2 * HASH_SIZE, 1, 1, 1> out_pkt_t;

// Input packet with pipeline control.
typedef struct {
    // Starting hash.
    ap_uint<256> hash;
    // Number of iterations of a hash function to be applied to the starting hash.
    ap_uint<64> num_iters;
    // This bit indicates a terminator packet that doesn't have to be hashed. Other fields are
    // ignored.
    ap_uint<1> terminator;
} in_pkt_ctrl_t;

ap_uint<HASH_SIZE> dummy_hash_iter(ap_uint<HASH_SIZE> hash, ap_uint<NUM_ITERS_SIZE> num_iters) {
    for (ap_uint<NUM_ITERS_SIZE> i = 0; i < num_iters; i++) {
        hash = hash + hash;
    }
    return hash;
}

void in_pkts_par(hls::stream<xdma_axis_t> &in_words,
                 hls::stream<in_pkt_t> &in_pkt_ctrls_par[IN_PKT_PAR]) {
    for (unsigned batch = 0; batch < IN_PKT_PAR / 8; i++) {
#pragma HLS pipeline
        in_pkt_t in_pkt;
        xdma_axis_t word;
        unsigned i = batch * 8;

        // Packet 0
        word = in_words.read();
        in_pkt = word(0, 319);
        in_pkts[i].write(in_pkt);
        if (word.last) break;

        // Packet 1
        in_pkt(0, 191) = word(320, 511);
        word = in_words.read();
        in_pkt(192, 319) = word(0, 127);
        in_pkts[i + 1].write(to_in_pkt_ctrl(in_pkt));
        if (word.last) break;

        // Packet 2
        in_pkt = word(128, 447);
        in_pkts[i + 2].write(to_in_pkt_ctrl(in_pkt));
        if (word.last) break;

        // Packet 3
        in_pkt(0, 63) = word(448, 511);
        word = in_words.read();
        in_pkt(64, 319) = word(0, 255);
        in_pkts[i + 3].write(to_in_pkt_ctrl(in_pkt));
        if (word.last) break;

        // Packet 4
        in_pkt(0, 255) = word(256, 511);
        word = in_words.read();
        in_pkt(256, 319) = word(0, 63);
        in_pkts[i + 4].write(to_in_pkt_ctrl(in_pkt));
        if (word.last) break;

        // Packet 5
        in_pkt = word(64, 383);
        in_pkts[i + 5].write(to_in_pkt_ctrl(in_pkt));
        if (word.last) break;

        // Packet 6
        in_pkt(0, 127) = word(384, 511);
        word = in_words.read();
        in_pkt(128, 319) = word(0, 191);
        in_pkts[i + 6].write(to_in_pkt_ctrl(in_pkt));
        if (word.last) break;

        // Packet 7
        in_pkt = word(192, 511);
        in_pkts[i + 7].write(to_in_pkt_ctrl(in_pkt));
        if (word.last) break;
    }

    in_pkt_ctrl_t terminator = {0, 0, 1};
    // in_pkt_ctrl.hash = 0;
    // in_pkt_ctrl.num_iters = 0;
    // in_pkt_ctrl.terminator = 1;
    // Terminate all stream processing tasks.
    for (unsigned i = 0; i < IN_PKT_PAR; i++) {
        in_pkts[i].write(terminator);
    }
}

in_pkt_ctrl_t to_in_pkt_ctrl(in_pkt_t in_pkt) {
    in_pkt_ctrl_t in_pkt_ctrl;
    in_pkt_ctrl.hash = in_pkt.data(HASH_SIZE - 1 + NUM_ITERS_SIZE, NUM_ITERS_SIZE);
    in_pkt_ctrl.num_iters = in_pkt.data(NUM_ITERS_SIZE - 1, 0);
    return in_pkt_ctrl;
}

void hash_iter_pkts(hls::stream<in_pkt_ctrl_t> &in_pkt_ctrls,
                    hls::stream<xdma_axis_t> &out_pkts) {
    in_pkt_ctrl_t in_pkt_ctrl;
    while (in_pkt_ctrl = in_pkt_ctrls.read(); !in_pkt_ctrl.terminator) {
// #pragma HLS pipeline
// #pragma HLS unroll factor=64 skip_exit_check
        ap_uint<HASH_SIZE> in_hash = in_pkt_ctrl.hash;
        ap_uint<HASH_SIZE> out_hash = dummy_hash_iter(in_hash, in_pkt_ctrl.num_iters);

        xdma_axis_t out_pkt;
        out_pkt.data(2 * HASH_SIZE - 1, HASH_SIZE) = in_hash;
        out_pkt.data(HASH_SIZE - 1, 0) = out_hash;

        out_pkts.write(out_pkt);
    }
}

void hash_iter_pkts_par(hls::stream<in_pkt_ctrl_t> &in_pkt_ctrls_par[IN_PKT_PAR],
                        hls::stream<xdma_axis_t> &out_pkts) {
    for (unsigned i = 0; i < IN_PKTS_PAR; i++) {
        hash_iter_pkts(in_pkt_ctrls_par[i], out_pkts);
    }
}

void hashes_iter(hls::stream<xdma_axis_t> &in_words,
                 hls::stream<xdma_axis_t> &out_words) {
#pragma HLS interface axis port=in_words
#pragma HLS interface axis port=out_words
#pragma HLS interface s_axilite port=return bundle=control
#pragma HLS dataflow

    hls::stream<in_pkt_ctrl_t> in_pkt_ctrls_par[IN_PKT_PAR];
    in_pkts_par(in_pkts, in_pkt_ctrls_par);
    hash_iter_pkts_par(in_pkt_ctrls_par, out_pkts);

    // TODO: termination of the output stream
}
