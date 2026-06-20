cmake_minimum_required(VERSION 3.24)

project(
  ni-random
  VERSION 0.1.0
  DESCRIPTION
    "Header-only C++20 library of random number engines, distributions, and seeding utilities"
  HOMEPAGE_URL "https://github.com/native-instruments/ni-random"
  LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
# When ni-random is the top-level project we default tests on and provide the
# usual developer affordances; when it is consumed via add_subdirectory /
# FetchContent the consumer's settings win and tests/benchmarks stay off.
include(CMakeDependentOption)

option(NI_RANDOM_BUILD_TESTS "Build the ni-random unit tests"
       ${PROJECT_IS_TOP_LEVEL})
option(NI_RANDOM_BUILD_BENCHMARKS "Build the ni-random benchmarks" OFF)
cmake_dependent_option(
  NI_RANDOM_INSTALL "Generate install rules and a package config for ni-random"
  ON "PROJECT_IS_TOP_LEVEL" OFF)

# ---------------------------------------------------------------------------
# The library: header-only INTERFACE target
# ---------------------------------------------------------------------------
add_library(ni-random INTERFACE)
add_library(ni-random::ni-random ALIAS ni-random)

target_compile_features(ni-random INTERFACE cxx_std_20)

# Expose include/ at build time and <prefix>/include after install. FILE_SET
# HEADERS lets the same declaration drive include dirs *and* header install,
# preserving the ni/random/ directory structure.
file(GLOB_RECURSE NI_RANDOM_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/include/ni/random/*.hpp")
target_sources(
  ni-random INTERFACE FILE_SET HEADERS BASE_DIRS
                      ${CMAKE_CURRENT_SOURCE_DIR}/include FILES
                      ${NI_RANDOM_HEADERS})

# ---------------------------------------------------------------------------
# Tests & benchmarks
# ---------------------------------------------------------------------------
if(NI_RANDOM_BUILD_TESTS OR NI_RANDOM_BUILD_BENCHMARKS)
  include(FetchContent)
endif()

if(NI_RANDOM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if(NI_RANDOM_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

# ---------------------------------------------------------------------------
# Install / export
# ---------------------------------------------------------------------------
if(NI_RANDOM_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(
    TARGETS ni-random
    EXPORT ni-random-targets
    FILE_SET HEADERS)

  install(
    EXPORT ni-random-targets
    NAMESPACE ni-random::
    FILE ni-random-targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ni-random)

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ni-random-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ni-random-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ni-random)

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ni-random-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ni-random-config.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/ni-random-config-version.cmake
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ni-random)
endif()
