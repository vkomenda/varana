#include "ap_axi_sdata.h"
#include "hls_stream.h"

typedef struct {
    ap_uint<256> hash;
    ap_uint<64> num_iters;
    ap_uint<1> last;
} in_pkt_t;

ap_uint<256> dummy_hash_iter(ap_uint<256> hash, ap_uint<64> num_iters) {
    for (ap_uint<64> i = 0; i < num_iters; i++) {
        hash = hash + hash;
    }
    return hash;
}

void hashes_iter(hls::stream<in_pkt_t> &in_pkts, hls::stream<ap_uint<256>> &out_hashes) {
#pragma HLS INTERFACE axis port=in_pkts
#pragma HLS INTERFACE axis port=out_hashes
// #pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

    in_pkt_t in_pkt;
    do {
#pragma HLS PIPELINE
        in_pkt = in_pkts.read();
        ap_uint<256> out_hash = dummy_hash_iter(in_pkt.hash, in_pkt.num_iters);
        out_hashes.write(out_hash);
    } while (!in_pkt.last);
}
