#ifndef __SYNTHESIS__
#include <iostream>
#endif

#include "ap_axi_sdata.h"
#include "hashes_iter.h"
#include "hls_stream.h"
#include "sha256.h"

typedef ap_axiu<IN_PKT_SIZE, 0, 0, 0> in_pkt_t;
// typedef ap_axiu<2 * HASH_SIZE, 1, 1, 1> out_pkt_t;

// Input packet with pipeline control.
typedef struct {
    // Starting hash.
    ap_uint<256> hash;
    // Number of iterations of a hash function to be applied to the starting hash.
    ap_uint<64> num_iters;
    // // This bit indicates a terminator packet that doesn't have to be hashed. Other fields are
    // // ignored.
    // ap_uint<1> terminator;
} in_pkt_ctrl_t;

// typedef struct {
//     // Cells contain either a free task number or -1 indicating that the cell is empty.
//     ap_int<IN_PKT_PAR_BITS + 1> cells[IN_PKT_PAR];
//     // Read cell index. Negative values mean "undefined".
//     ap_int<IN_PKT_PAR_BITS + 1> read_pos;
//     // Write cell index. Negative values mean "undefined".
//     ap_int<IN_PKT_PAR_BITS + 1> write_pos;
// } task_fifo_t;

// // The queue of available tasks.
// static hls::stream<unsigned> task_fifo;
// // In the first iteration through all tasks, the queue is not initialised yet. At the end of each
// // run, every task announces its availabilty by appending its index to the queue. Therefore at the
// // end of the first iteration the queue becomes initialised and `task_fifo_ready` latches to `true`.
// static bool task_fifo_ready = false;

// static task_fifo_t task_fifo;
// // RAW - read after write
// #pragma HLS dependence variable=task_fifo inter RAW false
// // RAM_S2P - A dual-port RAM that allows read operations on one port and write operations on the other port.
// // URAM - UltraRAM
// #pragma HLS bind_storage variable=task_fifo type=ram_s2p impl=uram

// ap_int<IN_PKT_PAR_BITS + 1> read_task_fifo() {
//     ap_int<IN_PKT_PAR_BITS + 1> read_pos = task_fifo.read_pos;
//     if (task_fifo.read_pos < 0) {
//         return -1;
//     }
//     ap_int<IN_PKT_PAR_BITS + 1> v = task_fifo.cells[read_pos];
//     task_fifo.cells[read_pos] = -1;
//     ap_int<IN_PKT_PAR_BITS + 2> next_read_pos = (read_pos + 1) % IN_PKT_PAR;
//     if (task_fifo.cells[next_read_pos] >= 0) {
//         task_fifo.read_pos = next_read_pos;
//     } else {
//         task_fifo.read_pos = -1;
//     }
//     if (task_fifo.write_pos < 0) {
//         task_fifo.write_pos = read_pos;
//     }
//     return v;
// }

ap_uint<HASH_SIZE> dummy_hash_iter(ap_uint<HASH_SIZE> hash, ap_uint<NUM_ITERS_SIZE> num_iters) {
#ifndef __SYNTHESIS__
    std::cout << "dummy_hash_iter("
              << hash.to_string(16, true).c_str() << ", "
              << num_iters.to_string(16, true).c_str() << ")" << std::endl;
#endif
    for (unsigned i = 0; i < num_iters; i++) {
        hash = hash + hash;
    }
    return hash;
}

in_pkt_ctrl_t to_in_pkt_ctrl(in_pkt_t &in_pkt) {
    in_pkt_ctrl_t in_pkt_ctrl;
    in_pkt_ctrl.hash = in_pkt.data(HASH_SIZE - 1 + NUM_ITERS_SIZE, NUM_ITERS_SIZE);
    in_pkt_ctrl.num_iters = in_pkt.data(NUM_ITERS_SIZE - 1, 0);
    return in_pkt_ctrl;
}

void submit_pkt(hls::stream<in_pkt_ctrl_t> in_pkt_ctrls_par[IN_PKT_PAR], in_pkt_t &in_pkt, unsigned seq_index) {
    // unsigned task_index;
    // if (task_fifo_ready) {
    //     task_index = task_fifo.read();
    // } else {
    //     task_index = seq_index;
    // }
    in_pkt_ctrls_par[seq_index].write(to_in_pkt_ctrl(in_pkt));
}

unsigned demux_in_pkts(hls::stream<xdma_axis_t> &in_words,
                       hls::stream<in_pkt_ctrl_t> in_pkt_ctrls_par[IN_PKT_PAR]) {
    xdma_axis_t word;
    bool last = false;
    unsigned num_packets = 0;

    // Iterate over maximum IN_PKT_PAR packets in 8-packet batches, terminating on a batch boundary,
    // which limits the number of termination condition checks within a batch to just one.
    for (unsigned batch = 0; batch < IN_PKT_PAR / 8 && !last; batch++) {
        // #pragma HLS pipeline
        in_pkt_t in_pkt;
        unsigned i = batch * 8;

        // for (unsigned j = 0; j < 5; j++) {
        //     word = in_words.read();
        //     std::cout << word.data.to_string(16, true).c_str() << std::endl;
        // }

        // Packet 0
        word = in_words.read();
        last = last | word.last;
        in_pkt.data = word.data(511, 192);
        submit_pkt(in_pkt_ctrls_par, in_pkt, i);

        // Packet 1
        in_pkt.data(319, 128) = word.data(191, 0);
        word = in_words.read();
        last = last | word.last;
        in_pkt.data(127, 0) = word.data(511, 384);
        submit_pkt(in_pkt_ctrls_par, in_pkt, i + 1);

        // Packet 2
        in_pkt.data = word.data(383, 64);
        submit_pkt(in_pkt_ctrls_par, in_pkt, i + 2);

        // Packet 3
        in_pkt.data(319, 256) = word.data(63, 0);
        word = in_words.read();
        last = last | word.last;
        in_pkt.data(255, 0) = word.data(511, 256);
        submit_pkt(in_pkt_ctrls_par, in_pkt, i + 3);

        // Packet 4
        in_pkt.data(319, 64) = word.data(255, 0);
        word = in_words.read();
        last = last | word.last;
        in_pkt.data(63, 0) = word.data(511, 448);
        submit_pkt(in_pkt_ctrls_par, in_pkt, i + 4);

        // Packet 5
        in_pkt.data = word.data(447, 128);
        submit_pkt(in_pkt_ctrls_par, in_pkt, i + 5);

        // Packet 6
        in_pkt.data(319, 192) = word.data(127, 0);
        word = in_words.read();
        last = last | word.last;
        in_pkt.data(191, 0) = word.data(511, 320);
        submit_pkt(in_pkt_ctrls_par, in_pkt, i + 6);

        // Packet 7
        in_pkt.data = word.data(319, 0);
        submit_pkt(in_pkt_ctrls_par, in_pkt, i + 7);

        num_packets += 8;
    }

    return num_packets;
}

void hash_iter_pkts(hls::stream<in_pkt_ctrl_t> &in_pkt_ctrls,
                    ap_uint<256> *gmem,
                    unsigned task_index) {
    in_pkt_ctrl_t in_pkt_ctrl = in_pkt_ctrls.read();
    ap_uint<HASH_SIZE> in_hash = in_pkt_ctrl.hash;
    ap_uint<HASH_SIZE> out_hash = dummy_hash_iter(in_hash, in_pkt_ctrl.num_iters);

    gmem[task_index * HASH_SIZE] = out_hash;
    // xdma_axis_t out_pkt;
    // out_pkt.data(XDMA_AXIS_WIDTH - 1, HASH_SIZE) = in_hash;
    // out_pkt.data(HASH_SIZE - 1, 0) = out_hash;
    // out_pkts.write(out_pkt);
    // Make the task available.
    // task_fifo.write(task_index);
}

void hash_iter_pkts_par(hls::stream<in_pkt_ctrl_t> in_pkt_ctrls_par[IN_PKT_PAR],
                        ap_uint<256> *gmem) {
    for (unsigned i = 0; i < IN_PKT_PAR; i++) {
#pragma HLS unroll
        hash_iter_pkts(in_pkt_ctrls_par[i], gmem, i);
    }
}

// void mux_out_pkts(hls::stream<ap_uint<XDMA_AXIS_WIDTH>> out_pkts_par[IN_PKT_PAR],
//                   hls::stream<xdma_axis_t> &out_words) {
//     ap_uint<1> buffered_pkts[IN_PKT_PAR];
//     for (unsigned i = 0; i < IN_PKT_PAR; i++) {
// #pragma HLS unroll
//         buffered_pkts[i] = 0;
//     }

//     for (unsigned i = 0; i < IN_PKT_PAR; i++) {
// #pragma HLS unroll
//         xdma_axis_t out_pkt;
//         out_pkt.data = out_pkts_par[i].read();
//         out_words.write(out_pkt);
//         // if (!out_words.write_nb(out_pkt)) {
//         //     unsigned offset = i * 2;
//         //     gmem[offset] = out_pkt.data(511, 256);
//         //     gmem[offset + 1] = out_pkt.data(255, 0);
//         // }
//     }

//     for (unsigned i = 0; i < IN_PKT_PAR; i++) {
//         // Not unrolling the loop because of blocking in `out_words.write`.
//         if (!buffered_pkts[i]) continue;

//         unsigned offset = i * 2;
//         xdma_axis_t out_pkt;
//         out_pkt.data(511, 256) = gmem[offset];
//         out_pkt.data(255, 0) = gmem[offset + 1];
//         buffered_pkts[i] = 0;
//         out_words.write(out_pkt);
//     }
// }

unsigned pkts_dataflow(hls::stream<xdma_axis_t> &in_words,
                       hls::stream<xdma_axis_t> &out_words,
                       ap_uint<256> *gmem) {
    hls::stream<in_pkt_ctrl_t> in_pkt_ctrls_par[IN_PKT_PAR];
    unsigned num_packets;

#pragma HLS stream variable=in_pkt_ctrls_par type=fifo depth=4

#pragma HLS dataflow
    num_packets = demux_in_pkts(in_words, in_pkt_ctrls_par);
    hash_iter_pkts_par(in_pkt_ctrls_par, gmem);
    //    mux_out_pkts(out_pkts_par, out_words);

    return num_packets;
}

void hashes_iter(hls::stream<xdma_axis_t> &in_words,
                 hls::stream<xdma_axis_t> &out_words,
                 ap_uint<256> *gmem) {
#pragma HLS interface axis port=in_words
#pragma HLS interface axis port=out_words
#pragma HLS interface ap_ctrl_none port=return
// #pragma HLS interface s_axilite port=return bundle=control
#pragma HLS interface m_axi port=gmem

    pkts_dataflow(in_words, out_words, gmem);

    for (unsigned i = 0; i < IN_PKT_PAR; i++) {
        unsigned offset = i * HASH_SIZE;
        xdma_axis_t out_pkt;
        out_pkt.data(511, 256) = gmem[offset];
        out_pkt.data(255, 0) = gmem[offset + 1];
        out_words.write(out_pkt);
    }
}

#ifndef __SYNTHESIS__
bool is_task_fifo_ready() {
    return false; // task_fifo_ready;
}

int take_task_fifo_head() {
    unsigned task_id;
    // if (task_fifo.read_nb(task_id)) {
    //     return task_id;
    // } else {
    //     return -1;
    // }
    return -1;
}
#endif
