cmake_minimum_required (VERSION 3.9)
project (MIPS32 CXX)

add_library(fs-mips32 SHARED
    src/ram.cpp
    src/ram_io.cpp
    src/mmu.cpp
    src/cp0.cpp
    src/cp1.cpp
    src/cpu.cpp
    src/machine_inspector.cpp
    src/machine.cpp
)

target_compile_features(fs-mips32 PRIVATE cxx_std_17)
target_include_directories(fs-mips32 PUBLIC include)
target_compile_options(fs-mips32 PRIVATE /W3 /fp:strict /wd4146 /wd4267 /permissive-)
target_compile_definitions(fs-mips32 PRIVATE "-D_CRT_SECURE_NO_WARNINGS")