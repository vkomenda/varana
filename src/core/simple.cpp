#include "ap_int.h"

int simple_square(ap_uint<256> *gmem, int x) {
#pragma HLS interface s_axilite bundle=ctrl port=x
#pragma HLS interface s_axilite bundle=ctrl port=return
#pragma HLS interface m_axi port=gmem

  return x * x;
}
