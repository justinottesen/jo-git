cmake_minimum_required(VERSION 4.0)
project(jo-git LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wno-unused -g)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src)

