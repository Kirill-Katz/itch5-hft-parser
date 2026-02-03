cmake_minimum_required(VERSION 3.20)
project(ITCH LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3 -march=native -g")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

find_package(absl REQUIRED)
find_package(benchmark REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_library(itch_parser INTERFACE)
target_include_directories(itch_parser INTERFACE include)
target_link_libraries(itch_parser INTERFACE absl::flat_hash_map)
target_link_libraries(itch_parser INTERFACE benchmark::benchmark pthread)

file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp")

add_executable(benchmark ${SRC_FILES})
target_link_libraries(benchmark PRIVATE itch_parser)
target_link_libraries(benchmark PRIVATE ${DPDK_LIBRARIES})

add_executable(perf_bench ${SRC_FILES})
target_link_libraries(perf_bench PRIVATE itch_parser)
target_link_libraries(perf_bench PRIVATE ${DPDK_LIBRARIES})
target_compile_definitions(perf_bench PRIVATE PERF)
