cmake_minimum_required(VERSION 3.19)
project(stfu)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_executable(stfu
        main.cc
        stfu.h expect.h)

