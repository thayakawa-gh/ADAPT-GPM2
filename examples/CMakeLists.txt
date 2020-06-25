include_directories(../)

add_executable(examples main.cpp example_2d.h example_colormap.h example_filledcurve.h example_string.h)

target_compile_options(examples PUBLIC -O2 -W3)