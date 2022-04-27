#include <ap_int.h>
#include <cstdint>
#include <tapa.h>

#define Ch(x, y, z)     (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x, y, z)    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define Sigma0(x)       (rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22))
#define Sigma1(x)       (rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25))
#define Gamma0(x)       (rotr(x, 7) ^ rotr(x, 18) ^ ((x) >> 3))
#define Gamma1(x)       (rotr(x, 17) ^ rotr(x, 19) ^ ((x) >> 10))

using state_t = struct state_t {
    ap_uint<32> a;
    ap_uint<32> b;
    ap_uint<32> c;
    ap_uint<32> d;
    ap_uint<32> e;
    ap_uint<32> f;
    ap_uint<32> g;
    ap_uint<32> h;
};

const ap_uint<32> H[8] = {
    0x6a09e667,
    0xbb67ae85,
    0x3c6ef372,
    0xa54ff53a,
    0x510e527f,
    0x9b05688c,
    0x1f83d9ab,
    0x5be0cd19
};

const ap_uint<32> K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
    0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
    0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
    0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
    0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
    0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
    0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
    0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

template<int S>
ap_uint<S> rotr(ap_uint<S> x, int n) {
#pragma HLS INLINE
    ap_uint<S> tmp = x;
    tmp.rrotate(n);
    return tmp;
}

static ap_uint<512> pad_message(ap_uint<256> msg) {
#pragma HLS INLINE
    ap_uint<512> tmp;
    tmp(511, 256) = msg;
    tmp[255] = 1;
    tmp(254, 64) = 0;
    tmp(63, 0) = 256;
    return tmp;
}

// Performs one round of SHA-256 compression.
static state_t compress_round(state_t state, ap_uint<32> rk, ap_uint<32> rw) {
// #pragma HLS bind_op variable=rk op=add impl=dsp
// #pragma HLS bind_op variable=rw op=add impl=dsp
    ap_uint<32> a = state.a;
    ap_uint<32> b = state.b;
    ap_uint<32> c = state.c;
    ap_uint<32> d = state.d;
    ap_uint<32> e = state.e;
    ap_uint<32> f = state.f;
    ap_uint<32> g = state.g;
    ap_uint<32> h = state.h;
// #pragma HLS bind_op variable=h op=add impl=dsp

    ap_uint<32> sigma1 = Sigma1(e);
// #pragma HLS bind_op variable=sigma1 op=add impl=dsp
    ap_uint<32> ch = Ch(e, f, g);
// #pragma HLS bind_op variable=ch op=add impl=dsp
    ap_uint<32> tmp1 = h + sigma1 + ch + rk + rw;
// #pragma HLS bind_op variable=tmp1 op=add impl=dsp
    ap_uint<32> sigma0 = Sigma0(a);
// #pragma HLS bind_op variable=sigma0 op=add impl=dsp
    ap_uint<32> maj = Maj(a, b, c);
// #pragma HLS bind_op variable=maj op=add impl=dsp
    ap_uint<32> tmp2 = sigma0 + maj;
// #pragma HLS bind_op variable=tmp2 op=add impl=dsp

    state.h = g;
    state.g = f;
    state.f = e;
    state.e = d + tmp1;
    state.d = c;
    state.c = b;
    state.b = a;
    state.a = tmp1 + tmp2;

    return state;
}

// Computes the digest of a 256-bit message padded to 512 bits.
static ap_uint<512> process_sha256(ap_uint<512> padded_msg) {
    ap_uint<32> w[64];
// #pragma HLS array_partition variable=w complete
// #pragma HLS bind_op variable=w op=add impl=dsp

    w[0] = padded_msg(511, 480);
    w[1] = padded_msg(479, 448);
    w[2] = padded_msg(447, 416);
    w[3] = padded_msg(415, 384);
    w[4] = padded_msg(383, 352);
    w[5] = padded_msg(351, 320);
    w[6] = padded_msg(319, 288);
    w[7] = padded_msg(287, 256);
    w[8] = padded_msg(255, 224);
    w[9] = padded_msg(223, 192);
    w[10] = padded_msg(191, 160);
    w[11] = padded_msg(159, 128);
    w[12] = padded_msg(127, 96);
    w[13] = padded_msg(95, 64);
    w[14] = padded_msg(63, 32);
    w[15] = padded_msg(31, 0);

 loop_apply_gammas:
    for (int i = 16; i < 64; i++) {
// #pragma HLS bind_op variable=i op=sub impl=dsp
        ap_uint<32> gamma0 = Gamma0(w[i - 15]);
        ap_uint<32> gamma1 = Gamma1(w[i - 2]);
// #pragma HLS bind_op variable=gamma0 op=add impl=dsp
// #pragma HLS bind_op variable=gamma1 op=add impl=dsp
        w[i] = w[i - 16] + gamma0 + w[i - 7] + gamma1;
    }

    state_t state;
    state.a = H[0];
    state.b = H[1];
    state.c = H[2];
    state.d = H[3];
    state.e = H[4];
    state.f = H[5];
    state.g = H[6];
    state.h = H[7];

 loop_apply_compress_round:
    for (int i = 0; i < 64; i++) {
        state = compress_round(state, K[i], w[i]);
    }

    state.a += H[0];
    state.b += H[1];
    state.c += H[2];
    state.d += H[3];
    state.e += H[4];
    state.f += H[5];
    state.g += H[6];
    state.h += H[7];

    return (state.a, state.b, state.c, state.d, state.e, state.f, state.g, state.h);
}

// Computes and returns the SHA-256 hash of the input 256-bit msg.
ap_uint<256> sha256(ap_uint<256> msg) {
    ap_uint<512> padded_msg = pad_message(msg);
    return process_sha256(padded_msg);
}

#define TAPA_WHILE_NEITHER_EOT_UNROLL(fifo1, fifo2, nk)                 \
    for (bool tapa_##fifo1##_valid, tapa_##fifo2##_valid;               \
         (!fifo1.eot(tapa_##fifo1##_valid) || !tapa_##fifo1##_valid) && \
         (!fifo2.eot(tapa_##fifo2##_valid) || !tapa_##fifo2##_valid);)  \
    _Pragma("HLS unroll factor=##nk##")                                 \
    _Pragma("HLS pipeline II = 1") if (tapa_##fifo1##_valid &&          \
                                       tapa_##fifo2##_valid)

constexpr int FIFO_DEPTH = 16;
// Number of hash kernels.
constexpr int NK = 128;

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

//     for (unsigned i = 0; i < 32; i++) {
// #pragma HLS unroll
//         unsigned char b = n((32 - i) * 8 - 1, (31 - i) * 8);
//         r((i + 1) * 8 - 1, i * 8) = b;
//     }

    return r;
}

void load_hashes(tapa::async_mmap<ap_uint<256>>& in_mmap,
                 uint32_t len,
                 tapa::ostreams<ap_uint<256>, NK> in_qs) {
    for (uint64_t i = 0, n_elem = 0; n_elem < len;) {
        ap_uint<256> elem;

        if (i < len && in_mmap.read_addr.try_write(i)) {
            i++;
        }
        if (in_mmap.read_data.try_read(elem)) {
            in_qs[i % NK].write(elem);
            n_elem++;
        }
    }

    for (uint64_t i = 0; i < NK; i++) {
        in_qs[i].close();
    }
}

void load_iters(tapa::async_mmap<uint64_t>& in_mmap,
                uint32_t len,
                tapa::ostreams<uint64_t, NK> in_qs) {
    for (uint64_t i = 0, n_elem = 0; n_elem < len;) {
        uint64_t elem;

        if (i < len && in_mmap.read_addr.try_write(i)) {
            i++;
        }
        if (in_mmap.read_data.try_read(elem)) {
            in_qs[i % NK].write(elem);
            n_elem++;
        }
    }

    for (uint64_t i = 0; i < NK; i++) {
        in_qs[i].close();
    }
}

void compute_kernel(tapa::istream<ap_uint<256>>& hashes,
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
            results.write(reverse_bytes_u256(h));
            hash_success = false;
            num_iter_success = false;
        }
    }
    results.close();
}

void compute(tapa::istreams<ap_uint<256>, NK>& hashes_qs,
             tapa::istreams<uint64_t, NK>& num_iters_qs,
             tapa::ostreams<ap_uint<256>, NK>& results_qs) {
    tapa::task().invoke<tapa::join, NK>(compute_kernel, hashes_qs, num_iters_qs, results_qs);
}

void store(tapa::istreams<ap_uint<256>, NK>& out_qs,
           tapa::mmap<ap_uint<256>> out_hashes) {
    for (uint64_t i = 0; i < NK; i++) {
        uint64_t j = 0;
        for (bool q_valid; !out_qs[i].eot(q_valid) || !q_valid;) {
#pragma HLS pipeline II = 1
            if (q_valid) {
                out_hashes[i + j * NK] = out_qs[i].read();
                j++;
            }
        }
    }
}

void poh(tapa::mmap<ap_uint<256>> in_hashes,
         tapa::mmap<uint64_t> num_iters,
         uint32_t num_hashes,
         tapa::mmap<ap_uint<256>> out_hashes) {
    tapa::streams<ap_uint<256>, NK, FIFO_DEPTH> in_hashes_qs("in_hashes_qs");
    tapa::streams<uint64_t, NK, FIFO_DEPTH> num_iters_qs("num_iters_qs");
    tapa::streams<ap_uint<256>, NK, FIFO_DEPTH> out_qs("out_qs");

    tapa::task()
        .invoke(load_hashes, in_hashes, num_hashes, in_hashes_qs)
        .invoke(load_iters, num_iters, num_hashes, num_iters_qs)
        .invoke(compute, in_hashes_qs, num_iters_qs, out_qs)
        .invoke(store, out_qs, out_hashes);
}
