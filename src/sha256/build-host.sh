g++ -o sha256-axi -O2 sha256-axi.cpp sha256-axi-host.cpp -l:libtapa.a -lfrt -l:libglog.so.0.0.0 -lgflags -lOpenCL -lpthread -ldl -lboost_context -I${HOME}/Xilinx/Vitis_HLS/2022.2/include -I/usr/local/include -g
# -fsanitize=address
