include_directories(../)

add_executable(examples main.cpp)

target_compile_options(examples PRIVATE
    $<$<CONFIG:Release>:-O2>
    $<$<CXX_COMPILER_ID:GCC>:-Wall>
    $<$<CXX_COMPILER_ID:Clang>:-Wall>
    $<$<CXX_COMPILER_ID:MSVC>:-W3>
)
target_compile_features(examples PRIVATE cxx_std_17)