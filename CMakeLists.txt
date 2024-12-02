cmake_minimum_required(VERSION 3.10)

# set the project name
project(simdhash VERSION 0.1)

# Set build type default Release
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# specify clang
SET(CMAKE_C_COMPILER clang)

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

# Set AVX flags for X86
message("Detected platform: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
if (NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    if (NOT AVX OR AVX STREQUAL "" OR AVX STREQUAL "256")
        target_compile_options(simdhash PUBLIC -mavx2)
    elseif(AVX STREQUAL "512")
        target_compile_options(simdhash PUBLIC -mavx512bw)
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
target_link_libraries(simdhash icuuc)

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