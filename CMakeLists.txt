
cmake_minimum_required (VERSION 3.9)

project ("mips32sim_demo" CXX)

set(CMAKE_CXX_EXTENSIONS NO)

add_subdirectory(mips32)
add_subdirectory(fmtlib)

add_executable(mips32sim_demo "mips32sim_demo.cpp")
target_compile_features(mips32sim_demo PRIVATE cxx_std_17)
target_link_libraries(mips32sim_demo PRIVATE fs-mips32 fmt::fmt)
target_compile_definitions(mips32sim_demo PRIVATE "_CRT_SECURE_NO_WARNINGS")

add_custom_command ( TARGET mips32sim_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:fmt::fmt> $<TARGET_FILE_DIR:mips32sim_demo>
)

add_custom_command ( TARGET mips32sim_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:fs-mips32> $<TARGET_FILE_DIR:mips32sim_demo>
)