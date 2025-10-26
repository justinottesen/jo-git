cmake_minimum_required(VERSION 4.0)
project(jo-git LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall -Wextra -Wpedantic -Werror -g -fdiagnostics-color=always
    -Wno-error=unused-variable
    -Wno-error=unused-parameter
    -Wno-error=unused-function
    -Wno-error=unused-local-typedefs
    -Wno-error=unused-but-set-variable
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wno-error=unused-private-field
    )
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
