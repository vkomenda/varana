PLATFORM=xilinx_u55n_gen3x4_xdma_1_202110_1
tapac -o sha256-axi.$PLATFORM.hw.xo sha256-axi.cpp --platform ~/Xilinx/platforms/$PLATFORM --top sha256_axi --work-dir sha256-axi.$PLATFORM.hw.xo.tapa
