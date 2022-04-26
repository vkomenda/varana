PLATFORM=xilinx_u55n_gen3x4_xdma_1_202110_1
tapac -o tapa_poh.$PLATFORM.hw.xo tapa_poh.cpp --platform ~/Xilinx/platforms/$PLATFORM --top poh --work-dir tapa_poh.$PLATFORM.hw.xo.tapa
