cmake_minimum_required(VERSION 3.10)

# set the project name
project(simdhash VERSION 0.1)

# specify clang
SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set compiler flags
set(STD_COMPILE_FLAGS "-Wall -Werror -Wno-unused-function -mavx2 -O3 -funroll-loops")
set(STD_LINKER_FLAGS "-mavx2")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STD_COMPILE_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STD_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${STD_LINKER_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -fno-omit-frame-pointer -fsanitize=address -DDEBUG")

# add the library
file(GLOB LIBSOURCES "./SimdHash/*.c")
add_library(simdhash ${LIBSOURCES})

# add the test
add_custom_target(simdhash_tests)
file(GLOB TESTS "./SimdHashTest/*.c")
foreach(TEST ${TESTS})
    add_definitions( -DTEST )
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g -ggdb")
    get_filename_component(TESTNAME ${TEST} NAME_WE)
    add_executable(${TESTNAME} EXCLUDE_FROM_ALL ${TEST} ${SOURCES})
    target_include_directories(${TESTNAME}
                                PUBLIC
                                    ./SimdHash/)
    target_link_libraries(${TESTNAME} simdhash)
    add_dependencies(simdhash_tests ${TESTNAME})
endforeach()