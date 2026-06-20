# Google Benchmark via FetchContent, pinned for reproducible CI.
set(BENCHMARK_ENABLE_TESTING
    OFF
    CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL
    OFF
    CACHE BOOL "" FORCE)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.1
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(benchmark)

# One benchmark bundle for all engines, mirroring the GYP benchmark target.
file(GLOB NI_RANDOM_BENCHMARK_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp")

add_executable(ni-random_benchmarks ${NI_RANDOM_BENCHMARK_SOURCES})

target_link_libraries(
  ni-random_benchmarks PRIVATE ni-random::ni-random benchmark::benchmark_main)

target_include_directories(ni-random_benchmarks
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
