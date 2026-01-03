cmake_minimum_required(VERSION 3.20)
project(SPMCQueue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -DNDEBUG -falign-loops=16 -g")

find_package(benchmark REQUIRED)
find_package(absl REQUIRED)
find_path(PAPI_INCLUDE_DIR
    NAMES papi.h
    PATHS /usr/include /usr/local/include
)

find_library(PAPI_LIBRARY
    NAMES papi
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
)

if (NOT PAPI_INCLUDE_DIR OR NOT PAPI_LIBRARY)
    message(FATAL_ERROR "PAPI not found")
endif()

add_library(PAPI::PAPI UNKNOWN IMPORTED)
set_target_properties(PAPI::PAPI PROPERTIES
    IMPORTED_LOCATION "${PAPI_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PAPI_INCLUDE_DIR}"
)


add_library(itch_parser INTERFACE)
target_include_directories(itch_parser INTERFACE include)
target_link_libraries(itch_parser INTERFACE absl::flat_hash_map)
target_link_libraries(itch_parser INTERFACE PAPI::PAPI)

add_executable(parser_bench
    benchmark/main.cpp
    src/utils.cpp
    benchmark/bm_parser_v1.cpp
    benchmark/bm_parser_v2.cpp
)
target_compile_options(parser_bench PRIVATE)
target_link_libraries(parser_bench PRIVATE itch_parser benchmark::benchmark)

add_executable(test
    src/main.cpp
)
target_compile_options(test PRIVATE -g -fno-omit-frame-pointer)
target_link_libraries(test PRIVATE itch_parser)
