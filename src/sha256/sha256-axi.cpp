#include <cstdint>
#include <tapa.h>
#include "sha256.cpp"

constexpr int FIFO_DEPTH = 16;

ap_uint<256> reverse_bytes_u256(ap_uint<256> n) {
#pragma HLS inline
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

    // for (uint8_t i = 0; i < 32; i++) {
    //     // #pragma HLS unroll
    //     uint8_t b = n((32 - i) * 8 - 1, (31 - i) * 8);
    //     r((i + 1) * 8 - 1, i * 8) = b;
    // }

    return r;
}

void load_hashes(tapa::async_mmap<ap_uint<256>>& mm,
                 uint32_t len,
                 tapa::ostream<ap_uint<256>>& q) {
    for (uint64_t i = 0, n_elem = 0; n_elem < len;) {
        ap_uint<256> elem;

        if (i < len && mm.read_addr.try_write(i)) {
            i++;
        }
        if (mm.read_data.try_read(elem)) {
            q.write(elem);
            n_elem++;
        }
    }

    q.close();
}

void load_iters(tapa::async_mmap<uint64_t>& mm,
                uint32_t len,
                tapa::ostream<uint64_t>& q) {
    for (uint64_t i = 0, n_elem = 0; n_elem < len;) {
        uint64_t elem;

        if (i < len && mm.read_addr.try_write(i)) {
            i++;
        }
        if (mm.read_data.try_read(elem)) {
            q.write(elem);
            n_elem++;
        }
    }

    q.close();
}

void compute(tapa::istream<ap_uint<256>>& hashes,
             tapa::istream<uint64_t>& num_iters,
             tapa::ostream<ap_uint<256>>& results) {
    bool hash_success = false;
    bool num_iter_success = false;
    ap_uint<256> hash;
    uint64_t num_iter;

    TAPA_WHILE_NEITHER_EOT(hashes, num_iters) {
        // Reads of in_hashes and num_iters happen in parallel. Any failing reads will be retried
        // until both reads are successful. Only then the hash function is applied.
        if (!hash_success) {
            hash = hashes.read(hash_success);
        }
        if (!num_iter_success) {
            num_iter = num_iters.read(num_iter_success);
        }
        if (hash_success && num_iter_success) {
            ap_uint<256> h = reverse_bytes_u256(hash);

        loop_apply_hash:
            for (uint64_t i = 0; i < num_iter; i++) {
                h = sha256(h);
            }
            ap_uint<256> result = reverse_bytes_u256(h);
            results.write(result);
            hash_success = false;
            num_iter_success = false;
        }
    }
    results.close();
}

void store(tapa::istream<ap_uint<256>>& q,
           tapa::mmap<ap_uint<256>> hashes) {
    // Sequential storage index.
    uint64_t m = 0;

    TAPA_WHILE_NOT_EOT(q) {
        hashes[m] = q.read();
        m++;
    }
}

void sha256_axi(tapa::mmap<ap_uint<256>> in_hashes,
                tapa::mmap<uint64_t> num_iters,
                uint32_t num_hashes,
                tapa::mmap<ap_uint<256>> out_hashes) {
    tapa::stream<ap_uint<256>, FIFO_DEPTH> in_hashes_q("in_hashes_q");
    tapa::stream<uint64_t, FIFO_DEPTH> num_iters_q("num_iters_q");
    tapa::stream<ap_uint<256>, FIFO_DEPTH> out_q("out_q");

    tapa::task()
        .invoke(load_hashes, in_hashes, num_hashes, in_hashes_q)
        .invoke(load_iters, num_iters, num_hashes, num_iters_q)
        .invoke(compute, in_hashes_q, num_iters_q, out_q)
        .invoke(store, out_q, out_hashes);
}
