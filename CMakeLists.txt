cmake_minimum_required(VERSION 3.8)

project(ADAPT-GPM2)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_subdirectory(examples)