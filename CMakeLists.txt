cmake_minimum_required(VERSION 3.14)
project (varana)

set(CMAKE_CXX_STANDARD 14)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/47f819c3ca54fb602f432904443e00a0a1fe2f42.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if (WIN32)
  set(VITIS_INCLUDE_PATH C:/Xilinx/Vitis_HLS/2021.2/include)
endif (WIN32)

if (UNIX)
  set(VITIS_INCLUDE_PATH ~/Xilinx/Vitis_HLS/2021.2/include)
endif (UNIX)

if (APPLE)
  set(VITIS_INCLUDE_PATH ~/Workspace/vitis_hls/include)
endif (APPLE)

include_directories(include ${VITIS_INCLUDE_PATH})

enable_testing()

add_executable(
  sha256_test
  ./test/sha256_test.cpp
  ./src/sha256/sha256.cpp
)

add_executable(
  hashes_iter_test
  ./test/hashes_iter_test.cpp
  ./src/hashes_iter/hashes_iter.cpp
  ./src/sha256/sha256.cpp
)

target_link_libraries(
  sha256_test
  gtest_main
)

target_link_libraries(
  hashes_iter_test
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(sha256_test)
gtest_discover_tests(hashes_iter_test)

set(FREQ 500)

function(add_vitis_hls_target name freq)
  add_custom_command(OUTPUT ${name}/${name}_f${freq}/syn/report/csynth.rpt COMMAND vitis_hls -f ../scripts/csynth.tcl ${name} ${freq})
  add_custom_command(OUTPUT ${name}/${name}_f${freq}/impl/export.zip COMMAND vitis_hls -f ../scripts/export.tcl ${name} ${freq} DEPENDS ${name}/${name}_f${freq}/syn/report/csynth.rpt)
  add_custom_target(csynth_${name}_f${freq} DEPENDS ${name}/${name}_f${freq}/syn/report/csynth.rpt)
  add_custom_target(export_${name}_f${freq} DEPENDS ${name}/${name}_f${freq}/impl/export.zip)
endfunction()

add_vitis_hls_target(sha256 ${FREQ})
add_vitis_hls_target(hashes_iter ${FREQ})
add_vitis_hls_target(poh ${FREQ})
