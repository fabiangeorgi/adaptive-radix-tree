cmake_minimum_required(VERSION 3.16)
project(art)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

option(CI_BUILD "Set to ON for complete build in CI." OFF)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type specified. Defaulting to Debug.
                    Use cmake `-DCMAKE_BUILD_TYPE=Release ..` to build an optimized version.")
endif()

if (CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "Building an optimized release version.")
    add_compile_options(-O3)
    if (CI_BUILD)
        # We cannot set this generally, as it does not work on all architectures.
        message(STATUS "Adding -march/tune=native in CI")
        add_compile_options(-march=native -mtune=native)
    endif()
endif()

set(TASK_SOURCES src/art.cpp src/art.hpp src/key.hpp)

add_library(art ${TASK_SOURCES})
target_include_directories(art INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Pass all available SIMD options on our server to GCC. Disable warning if unused.
target_compile_options(art PUBLIC
        -mmmx -msse -msse2 -msse3 -mssse3 -msse4 -msse4a -msse4.1 -msse4.2 -mavx
        -mavx2 -mavx512f -mavx512cd -mavx512vl -mavx512bw -mavx512dq -mavx512vnni)

enable_testing()
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.13.0
)

set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(basic_test test/basic.cpp)
add_test(basic_test basic_test)
target_link_libraries(basic_test art gtest gmock)

if (${CI_BUILD})
    # Build advanced tests in CI only
    add_executable(advanced_test test/advanced.cpp)
    add_test(advanced_test advanced_test)
    target_link_libraries(advanced_test art gtest gmock)

    add_executable(hdp_benchmark test/benchmark.cpp)
    target_link_libraries(hdp_benchmark art)
endif()
