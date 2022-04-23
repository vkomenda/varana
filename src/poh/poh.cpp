#include "poh.h"

ap_uint<256> reverse_bytes_u256(ap_uint<256> n) {
    ap_uint<256> r;
    r(255, 248) = n(7, 0);
    r(247, 240) = n(15, 8);
    r(239, 232) = n(23, 16);
    r(231, 224) = n(31, 24);
    r(223, 216) = n(39, 32);
    r(215, 208) = n(47, 40);
    r(207, 200) = n(55, 48);
    r(199, 192) = n(63, 56);
    r(191, 184) = n(71, 64);
    r(183, 176) = n(79, 72);
    r(175, 168) = n(87, 80);
    r(167, 160) = n(95, 88);
    r(159, 152) = n(103, 96);
    r(151, 144) = n(111, 104);
    r(143, 136) = n(119, 112);
    r(135, 128) = n(127, 120);
    r(127, 120) = n(135, 128);
    r(119, 112) = n(143, 136);
    r(111, 104) = n(151, 144);
    r(103, 96) = n(159, 152);
    r(95, 88) = n(167, 160);
    r(87, 80) = n(175, 168);
    r(79, 72) = n(183, 176);
    r(71, 64) = n(191, 184);
    r(63, 56) = n(199, 192);
    r(55, 48) = n(207, 200);
    r(47, 40) = n(215, 208);
    r(39, 32) = n(223, 216);
    r(31, 24) = n(231, 224);
    r(23, 16) = n(239, 232);
    r(15, 8) = n(247, 240);
    r(7, 0) = n(255, 248);

//     for (unsigned i = 0; i < 32; i++) {
// #pragma HLS unroll
//         unsigned char b = n((32 - i) * 8 - 1, (31 - i) * 8);
//         r((i + 1) * 8 - 1, i * 8) = b;
//     }

    return r;
}

void reverse_bytes_u256_array(ap_uint<256> *arr, unsigned len) {
 loop_reverse_hash_bytes:
    for (unsigned i = 0; i < len; i++) {
        arr[i] = reverse_bytes_u256(arr[i]);
    }
}

void poh(const ap_uint<256> *in_hashes, // input hashes
         const ap_uint<64> *num_iters,  // input numbers of iterations for each input hash
         unsigned num_hashes,           // input number of hashes (the number of elements in in_hashes and in num_iters)
         ap_uint<256> *out_hashes       // output hashes
) {
#pragma HLS interface m_axi port=in_hashes bundle=gmem max_read_burst_length=640
#pragma HLS interface m_axi port=num_iters bundle=gmem max_read_burst_length=640
#pragma HLS interface m_axi port=out_hashes bundle=gmem max_write_burst_length=640
#pragma HLS interface s_axilite port=in_hashes bundle=control
#pragma HLS interface s_axilite port=num_iters bundle=control
#pragma HLS interface s_axilite port=out_hashes bundle=control
#pragma HLS interface s_axilite port=num_hashes bundle=control
#pragma HLS interface s_axilite port=return bundle=control

    ap_uint<256> in_hashes_batch[BATCH_NUM_HASHES];
    ap_uint<64> num_iters_batch[BATCH_NUM_HASHES];
    ap_uint<256> out_hashes_batch[BATCH_NUM_HASHES];
// #pragma HLS bind_storage variable=in_hashes_batch type=ram_s2p impl=uram
// #pragma HLS bind_storage variable=out_hashes_batch type=ram_s2p impl=uram
// #pragma HLS bind_storage variable=num_iters_batch type=ram_1p impl=uram

 loop_process_batch:
    for (unsigned i = 0; i < num_hashes; i += BATCH_NUM_HASHES) {
        unsigned batch_num_hashes = BATCH_NUM_HASHES;
        bool tail_batch = false;
        if (i + BATCH_NUM_HASHES > num_hashes) {
            batch_num_hashes = num_hashes - i;
            tail_batch = true;
        }

    loop_load_in_hashes:
        for (unsigned j = 0; j < batch_num_hashes; j++) {
            in_hashes_batch[j] = in_hashes[i + j];
        }
    loop_load_num_iters:
        for (unsigned j = 0; j < batch_num_hashes; j++) {
            num_iters_batch[j] = num_iters[i + j];
        }

        // Big-endian -> little-endian
        reverse_bytes_u256_array(in_hashes_batch, batch_num_hashes);

        // Iterate a constant number of times in order to fully unroll this loop.
    loop_hash_batch:
        for (unsigned j = 0; j < BATCH_NUM_HASHES; j++) {
#pragma HLS unroll
            if (!tail_batch || j < batch_num_hashes) {
                ap_uint<256> out_hash = in_hashes_batch[j];
                ap_uint<64> num_iters_j = num_iters_batch[j];
                // FIFOs for communication with the SHA-256 kernel.
                ap_uint<256> msg[1];
                ap_uint<256> result[1];

            loop_apply_hash:
                for (unsigned k = 0; k < num_iters_j; k++) {
                    msg[0] = out_hash;
                    sha256(msg, result);
                    out_hash = result[0];
                }
                out_hashes_batch[j] = out_hash;
            }
        }

        // Little-endian -> big-endian
        reverse_bytes_u256_array(out_hashes_batch, batch_num_hashes);

    loop_store_out_hashes:
        for (unsigned j = 0; j < batch_num_hashes; j++) {
            out_hashes[i + j] = out_hashes_batch[j];
#ifndef __SYNTHESIS__
            std::cout << "out_hashes[" << i + j << "] = "
                      << out_hashes[i + j].to_string(16, true).c_str() << std::endl;
#endif
        }
    }
}
