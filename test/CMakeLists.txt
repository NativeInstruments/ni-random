# GoogleTest via FetchContent, pinned for reproducible CI.
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0
  GIT_SHALLOW TRUE)
# Don't override the parent project's compiler/linker runtime on Windows.
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
# GoogleTest is a build-time-only dependency; keep its headers/libs out of our
# install tree.
set(INSTALL_GTEST
    OFF
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# One test bundle for all engines/distributions, mirroring the GYP test target.
# The shared *.hpp suites are header-only; only *_tests.cpp carry test bodies.
file(GLOB NI_RANDOM_TEST_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*_tests.cpp")

add_executable(ni-random_tests ${NI_RANDOM_TEST_SOURCES})

target_link_libraries(ni-random_tests PRIVATE ni-random::ni-random
                                              GTest::gtest_main)

# Shared suite/traits headers are included by sibling, so the test dir is on the
# include path.
target_include_directories(ni-random_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(ni-random_tests)
