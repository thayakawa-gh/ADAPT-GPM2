include_directories(../)

add_executable(examples main.cpp)

target_compile_options(examples PUBLIC
  $<$<CONFIG:Release>:-O2 -W3>
  $<$<CONFIG:Debug>:-W3>
)
target_compile_features(examples PUBLIC cxx_std_17)