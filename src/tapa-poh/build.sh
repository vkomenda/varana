PLATFORM=xilinx_u55n_gen3x4_xdma_1_202110_1
tapac -o tapa-poh.$PLATFORM.hw.xo tapa-poh.cpp --platform ~/Xilinx/platforms/$PLATFORM --top poh --work-dir tapa-poh.$PLATFORM.hw.xo.tapa
