g++ -o tapa-poh -O2 tapa-poh.cpp tapa-poh-host.cpp -l:libtapa.a -lfrt -l:libglog.so.0.4.0 -lgflags -lOpenCL -lpthread -ldl -lboost_context -I${HOME}/Xilinx/Vitis_HLS/2022.1/include
