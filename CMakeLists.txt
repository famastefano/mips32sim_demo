
cmake_minimum_required (VERSION 3.9)

project ("mips32sim_demo" CXX)

set(CMAKE_CXX_EXTENSIONS NO)

add_subdirectory(mips32)
add_subdirectory(fmtlib)

add_executable(mips32sim_demo "mips32sim_demo.cpp")
target_compile_features(mips32sim_demo PRIVATE cxx_std_17)
target_link_libraries(mips32sim_demo PRIVATE fs-mips32 fmt::fmt)
target_compile_definitions(mips32sim_demo PRIVATE "_CRT_SECURE_NO_WARNINGS")