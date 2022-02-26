#include <hls_stream.h>
#include "sha256.h"

// #define NUM_PARALLEL_SHA256 8

// void engine(hls::stream<ap_uint<256>> &stream_in, hls::stream<ap_uint<256>> &stream_out) {
// #pragma HLS interface ap_ctrl_chain port=return
// #pragma HLS dataflow
// #pragma HLS interface ap_fifo port stream_in

//     for (int i = 0; i < NUM_PARALLEL_SHA256; i++) {
// #pragma HLS pipeline ii=1
//         ap_uint<256> msg = stream_in.read();
//         stream_out.write(sha256(msg));
//     }
// }

// HBM ADDR BITS
// [4-0]   : Do not use (not byte addressable)
// [5]     : Bank group bit 0
// [10-6]  : Column addr
// [12-11] : Bank addr
// [13]    : Bank group bit 1
// [27-14] : Row addr
// [30-28] : MC addr
// [31]    : Stack addr

#define BATCH_SIZE 16777216

void load(hls::stream<ap_uint<256>> &fifo, const ap_uint<256> *port) {
    for (int i = 0; i < BATCH_SIZE; i++) {
#pragma HLS pipeline ii=1
        fifo.write(port[i]);
    }
}

void store(hls::stream<ap_uint<256>> &fifo, ap_uint<256> *port) {
    for (int i = 0; i < BATCH_SIZE; i++) {
#pragma HLS pipeline ii=1
        ap_uint<256> v = fifo.read();
        port[i] = sha256(v);
    }
}

void engine(const ap_uint<256> *load_port, ap_uint<256> *store_port) {
#pragma HLS top
#pragma HLS interface mode=s_axilite port=return bundle=ctrl
#pragma HLS interface mode=ap_ctrl_chain port=return
#pragma HLS interface mode=s_axilite port=return bundle=ctrl
#pragma HLS interface mode=m_axi port=load_port offset=off max_read_burst_length=16 max_write_burst_length=16
#pragma HLS interface mode=m_axi port=store_port offset=off max_read_burst_length=16 max_write_burst_length=16
    for (int i = 0; i < 256; i++) {
#pragma HLS dataflow
        hls::stream<hash_pkt_t, 256> fifo;
        load_hash(fifo, load_port);
        store_hash(fifo, store_port);
    }
}
