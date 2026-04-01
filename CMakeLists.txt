cmake_minimum_required(VERSION 3.14)

# specify clang (must be before project() to avoid infinite reconfigure loop)
SET(CMAKE_C_COMPILER /usr/bin/clang)
SET(CMAKE_CXX_COMPILER /usr/bin/clang++)

# set the project name
project(simdhash VERSION 0.1 LANGUAGES C CXX)

# Set build type default Release
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Werror -fno-exceptions -Wunsafe-buffer-usage")
set(CMAKE_C_FLAGS "-Wall -Werror -Wno-unused-function")
set(CMAKE_C_FLAGS_RELEASE "-O3 -funroll-loops")
set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -O0 -fno-omit-frame-pointer -fsanitize=address -DDEBUG")

# Handle MacOS Homebrew
if(APPLE)
    execute_process(COMMAND brew --prefix OUTPUT_VARIABLE HOMEBREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    message("Homebrew prefix: ${HOMEBREW_PREFIX}")
    include_directories(${HOMEBREW_PREFIX}/include)
    link_directories(${HOMEBREW_PREFIX}/lib)
endif()

# add the library
file(GLOB LIBSOURCES "./src/*.c")

add_library(simdhash ${LIBSOURCES})

# Allow deprecated algorithms (MD4)
target_compile_options(simdhash PRIVATE -Wno-deprecated-declarations)

# Backward compat: map old AVX variable to new SIMD variable
if (NOT SIMD OR SIMD STREQUAL "")
    if (AVX STREQUAL "512")
        set(SIMD "avx512")
    elseif(AVX STREQUAL "256")
        set(SIMD "avx256")
    endif()
endif()

# Set SIMD flags for X86
message("Detected platform: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
if (NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64" AND NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
    if (SIMD STREQUAL "avx512")
        target_compile_options(simdhash PUBLIC -mavx512bw)
    elseif(SIMD STREQUAL "avx256")
        target_compile_options(simdhash PUBLIC -mavx2)
    elseif(SIMD STREQUAL "sse42")
        target_compile_options(simdhash PUBLIC -msse4.2)
    elseif(SIMD STREQUAL "ssse3")
        target_compile_options(simdhash PUBLIC -mssse3)
    elseif(SIMD STREQUAL "sse2")
        target_compile_options(simdhash PUBLIC -msse2)
    else()
        # Default: auto-detect from host CPU
        target_compile_options(simdhash PUBLIC -march=native)
    endif()
endif()

if(APPLE)
    # Need to specify homebrew icu paths for icu4c
    execute_process(COMMAND brew --prefix icu4c OUTPUT_VARIABLE HOMEBREW_ICU_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    message("Homebrew ICU prefix: ${HOMEBREW_ICU_PREFIX}")
    target_include_directories(simdhash PUBLIC ${HOMEBREW_ICU_PREFIX}/include)
    target_link_directories(simdhash PUBLIC ${HOMEBREW_ICU_PREFIX}/lib)
endif()

# Link icuuc
target_link_libraries(simdhash icuuc crypto)

# add the test
add_custom_target(simdhash_tests)
file(GLOB TESTS "./test/*.c")
foreach(TEST ${TESTS})
    add_definitions( -DTEST )
    get_filename_component(TESTNAME ${TEST} NAME_WE)
    add_executable(${TESTNAME} EXCLUDE_FROM_ALL ${TEST} ${SOURCES})
    target_include_directories(${TESTNAME}
                                PUBLIC
                                    ./src/)
    target_link_libraries(${TESTNAME} crypto simdhash)
    add_dependencies(simdhash_tests ${TESTNAME})
endforeach()

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Suppress project warnings for third-party gtest code
set(SAVED_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-w")
FetchContent_MakeAvailable(googletest)
set(CMAKE_CXX_FLAGS "${SAVED_CXX_FLAGS}")

enable_testing()

file(GLOB GTESTS "./test/*_test.cpp")
foreach(GTEST ${GTESTS})
    get_filename_component(GTESTNAME ${GTEST} NAME_WE)
    add_executable(${GTESTNAME} ${GTEST})
    target_include_directories(${GTESTNAME} PUBLIC ./src/)
    target_link_libraries(${GTESTNAME} gtest_main simdhash crypto)
    target_compile_options(${GTESTNAME} PRIVATE -Wno-unsafe-buffer-usage)
    add_test(NAME ${GTESTNAME} COMMAND ${GTESTNAME})
endforeach()