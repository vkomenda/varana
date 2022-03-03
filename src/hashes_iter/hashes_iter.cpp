#include "ap_axi_sdata.h"
#include "hashes_iter.h"
#include "hls_stream.h"

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

in_pkt_ctrl_t to_in_pkt_ctrl(in_pkt_t in_pkt) {
    in_pkt_ctrl_t in_pkt_ctrl;
    in_pkt_ctrl.hash = in_pkt.data(HASH_SIZE - 1 + NUM_ITERS_SIZE, NUM_ITERS_SIZE);
    in_pkt_ctrl.num_iters = in_pkt.data(NUM_ITERS_SIZE - 1, 0);
    return in_pkt_ctrl;
}

void in_pkts_par(hls::stream<xdma_axis_t> &in_words,
                 hls::stream<in_pkt_ctrl_t> in_pkt_ctrls_par[IN_PKT_PAR]) {
    xdma_axis_t word;
    do {
        for (unsigned batch = 0; batch < IN_PKT_PAR / 8; batch++) {
#pragma HLS pipeline
            in_pkt_t in_pkt;
            unsigned i = batch * 8;

            // Packet 0
            word = in_words.read();
            in_pkt.data = word.data(0, 319);
            in_pkt_ctrls_par[i].write(to_in_pkt_ctrl(in_pkt));
            if (word.last) break;

            // Packet 1
            in_pkt.data(0, 191) = word.data(320, 511);
            word = in_words.read();
            in_pkt.data(192, 319) = word.data(0, 127);
            in_pkt_ctrls_par[i + 1].write(to_in_pkt_ctrl(in_pkt));
            if (word.last) break;

            // Packet 2
            in_pkt.data = word.data(128, 447);
            in_pkt_ctrls_par[i + 2].write(to_in_pkt_ctrl(in_pkt));
            if (word.last) break;

            // Packet 3
            in_pkt.data(0, 63) = word.data(448, 511);
            word = in_words.read();
            in_pkt.data(64, 319) = word.data(0, 255);
            in_pkt_ctrls_par[i + 3].write(to_in_pkt_ctrl(in_pkt));
            if (word.last) break;

            // Packet 4
            in_pkt.data(0, 255) = word.data(256, 511);
            word = in_words.read();
            in_pkt.data(256, 319) = word.data(0, 63);
            in_pkt_ctrls_par[i + 4].write(to_in_pkt_ctrl(in_pkt));
            if (word.last) break;

            // Packet 5
            in_pkt.data = word.data(64, 383);
            in_pkt_ctrls_par[i + 5].write(to_in_pkt_ctrl(in_pkt));
            if (word.last) break;

            // Packet 6
            in_pkt.data(0, 127) = word.data(384, 511);
            word = in_words.read();
            in_pkt.data(128, 319) = word.data(0, 191);
            in_pkt_ctrls_par[i + 6].write(to_in_pkt_ctrl(in_pkt));
            if (word.last) break;

            // Packet 7
            in_pkt.data = word.data(192, 511);
            in_pkt_ctrls_par[i + 7].write(to_in_pkt_ctrl(in_pkt));
            if (word.last) break;
        }
    } while (!word.last);

    in_pkt_ctrl_t terminator = {0, 0, 1};
    // in_pkt_ctrl.hash = 0;
    // in_pkt_ctrl.num_iters = 0;
    // in_pkt_ctrl.terminator = 1;
    // Terminate all stream processing tasks.
    for (unsigned i = 0; i < IN_PKT_PAR; i++) {
#pragma HLS unroll
        // TODO: wait on busy streams.
        in_pkt_ctrls_par[i].write(terminator);
    }
}

void hash_iter_pkts(hls::stream<in_pkt_ctrl_t> &in_pkt_ctrls,
                    hls::stream<xdma_axis_t> &out_pkts) {
    in_pkt_ctrl_t in_pkt_ctrl;
    for (in_pkt_ctrl = in_pkt_ctrls.read();
         in_pkt_ctrl.terminator == 0;
         in_pkt_ctrl = in_pkt_ctrls.read()) {
// #pragma HLS unroll factor=64 skip_exit_check
        ap_uint<HASH_SIZE> in_hash = in_pkt_ctrl.hash;
        ap_uint<HASH_SIZE> out_hash = dummy_hash_iter(in_hash, in_pkt_ctrl.num_iters);

        xdma_axis_t out_pkt;
        out_pkt.data(2 * HASH_SIZE - 1, HASH_SIZE) = in_hash;
        out_pkt.data(HASH_SIZE - 1, 0) = out_hash;

        out_pkts.write(out_pkt);
    }
}

void hash_iter_pkts_par(hls::stream<in_pkt_ctrl_t> in_pkt_ctrls_par[IN_PKT_PAR],
                        hls::stream<xdma_axis_t> &out_pkts) {
    for (unsigned i = 0; i < IN_PKT_PAR; i++) {
#pragma HLS unroll
        hash_iter_pkts(in_pkt_ctrls_par[i], out_pkts);
    }
}

void pkts_dataflow(hls::stream<xdma_axis_t> &in_words,
                   hls::stream<xdma_axis_t> &out_words) {
    hls::stream<in_pkt_ctrl_t> in_pkt_ctrls_par[IN_PKT_PAR];
#pragma HLS stream variable=in_pkt_ctrls_par type=fifo depth=4
#pragma HLS dataflow
    in_pkts_par(in_words, in_pkt_ctrls_par);
    hash_iter_pkts_par(in_pkt_ctrls_par, out_words);
}

void hashes_iter(hls::stream<xdma_axis_t> &in_words,
                 hls::stream<xdma_axis_t> &out_words,
                 ap_uint<256> *gmem) {
#pragma HLS interface axis port=in_words
#pragma HLS interface axis port=out_words
#pragma HLS interface m_axi port=gmem
#pragma HLS interface s_axilite port=return bundle=control

    pkts_dataflow(in_words, out_words);

    // TODO: termination of the output stream

    gmem[0] = 0xc0ffee;
}
