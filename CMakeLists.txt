cmake_minimum_required(VERSION 3.29)

project(pcpow)

set(CMAKE_CXX_STANDARD 20)
add_executable(pcpow pce/PGE_TU.cpp
  pce/Basics.cpp
  pce/Bin.cpp
  pce/Clk.cpp
  pce/Dbg.cpp
  pce/Hu6.cpp
  pce/HwPage.cpp
  pce/Irq.cpp
  pce/Main.cpp
  pce/Pce.cpp
  pce/PceOut.cpp
  pce/Vce.cpp
  pce/Vdc.cpp
  pce/Vram.cpp
)
target_include_directories(pcpow PRIVATE ${CMAKE_SOURCE_DIR})
target_precompile_headers(pcpow PRIVATE third_party/pge/olcPixelGameEngine.h)
target_link_libraries(pcpow shell32)