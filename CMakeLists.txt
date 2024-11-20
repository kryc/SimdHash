cmake_minimum_required(VERSION 3.10)

# set the project name
project(simdhash VERSION 0.1)

# specify clang
SET(CMAKE_C_COMPILER clang)

# set compiler flags
message("Detected platform: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
if (NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    if (NOT AVX OR AVX STREQUAL "" OR AVX STREQUAL "256")
        set(AVXFLAG "-mavx2" CACHE STRING "" FORCE)
    elseif(AVX STREQUAL "512")
        set(AVXFLAG "-mavx512bw" CACHE STRING "" FORCE)
    endif()
endif()

set(CMAKE_C_FLAGS "-Wall -Werror -Wno-unused-function ${AVXFLAG}")
set(CMAKE_EXE_LINKER_FLAGS "${AVXFLAG} -lm -licuuc")

set(CMAKE_C_FLAGS_RELEASE "-O3 -funroll-loops")
set(CMAKE_C_FLAGS_DEBUG "-g -ggdb -O0 -fno-omit-frame-pointer -fsanitize=address -DDEBUG")

# add the library
file(GLOB LIBSOURCES "./src/*.c")
add_library(simdhash ${LIBSOURCES})

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
    target_link_libraries(${TESTNAME} simdhash)
    add_dependencies(simdhash_tests ${TESTNAME})
endforeach()