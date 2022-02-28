#include "ap_axi_sdata.h"
#include "hls_stream.h"

#define HASH_SIZE 256
#define NUM_ITERS_SIZE 64

typedef ap_axiu<HASH_SIZE + NUM_ITERS_SIZE, 1, 1, 1> in_pkt_t;
typedef ap_axiu<2 * HASH_SIZE, 1, 1, 1> out_pkt_t;

typedef struct {
    ap_uint<256> hash;
    ap_uint<64> num_iters;
    ap_uint<1> last;
} parsed_in_pkt_t;

ap_uint<HASH_SIZE> dummy_hash_iter(ap_uint<HASH_SIZE> hash, ap_uint<NUM_ITERS_SIZE> num_iters) {
    for (ap_uint<NUM_ITERS_SIZE> i = 0; i < num_iters; i++) {
        hash = hash + hash;
    }
    return hash;
}

void parse_in_pkts(hls::stream<in_pkt_t> &in_pkts,
                   hls::stream<parsed_in_pkt_t> &parsed_in_pkts) {
    in_pkt_t in_pkt;
    do {
#pragma HLS pipeline
        in_pkt = in_pkts.read();
        ap_uint<HASH_SIZE> in_hash = in_pkt.data(HASH_SIZE - 1 + NUM_ITERS_SIZE, NUM_ITERS_SIZE);
        ap_uint<NUM_ITERS_SIZE> num_iters = in_pkt.data(NUM_ITERS_SIZE - 1, 0);
        parsed_in_pkts.write({in_hash, num_iters, in_pkt.last});
    } while (!in_pkt.last);
}

void hash_iter_pkts(hls::stream<parsed_in_pkt_t> &parsed_in_pkts,
                    hls::stream<out_pkt_t> &out_pkts) {
    parsed_in_pkt_t parsed_in_pkt;
    do {
#pragma HLS pipeline
#pragma HLS unroll factor=64 skip_exit_check
        parsed_in_pkt = parsed_in_pkts.read();
        ap_uint<HASH_SIZE> in_hash = parsed_in_pkt.hash;
        ap_uint<HASH_SIZE> out_hash = dummy_hash_iter(in_hash, parsed_in_pkt.num_iters);

        out_pkt_t out_pkt;
        out_pkt.last = parsed_in_pkt.last;
        out_pkt.data(2 * HASH_SIZE - 1, HASH_SIZE) = in_hash;
        out_pkt.data(HASH_SIZE - 1, 0) = out_hash;

        out_pkts.write(out_pkt);
    } while (!parsed_in_pkt.last);

}


void hashes_iter(hls::stream<in_pkt_t> &in_pkts,
                 hls::stream<out_pkt_t> &out_pkts) {
#pragma HLS interface axis port=in_pkts
#pragma HLS interface axis port=out_pkts
#pragma HLS interface s_axilite port=return bundle=control
#pragma HLS dataflow

    hls::stream<parsed_in_pkt_t> parsed_in_pkts;
    parse_in_pkts(in_pkts, parsed_in_pkts);
    hash_iter_pkts(parsed_in_pkts, out_pkts);
}
