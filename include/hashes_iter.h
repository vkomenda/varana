#pragma once

#include <iostream>
#include "ap_axi_sdata.h"
#include "hls_stream.h"

#define XDMA_AXIS_WIDTH 512
#define HASH_SIZE 256
#define NUM_ITERS_SIZE 64
#define IN_PKT_SIZE (HASH_SIZE + NUM_ITERS_SIZE)
// The number of parallel input packets in the processing pipeline. Its product with IN_PKT_SIZE must be
// evenly divisible by XDMA_AXIS_WIDTH.
#define IN_PKT_PAR 64
#define IN_PKT_PAR_BITS 6

typedef ap_axiu<XDMA_AXIS_WIDTH, 0, 0, 0> xdma_axis_t;

extern void hashes_iter(hls::stream<xdma_axis_t> &in_words,
                        hls::stream<xdma_axis_t> &out_words,
                        ap_uint<256> *gmem);
