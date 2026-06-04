cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.10)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# project information
project(FuzzTxParser
        VERSION 1.0
	      DESCRIPTION "Fuzzing of transaction parser"
        LANGUAGES C)

# guard against bad build-type strings
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Fuzzer needs to be built with Clang")
endif()

if (NOT DEFINED BOLOS_SDK)
  message(FATAL_ERROR "BOLOS_SDK environment variable not found.")
endif()

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

# compatible with ClusterFuzzLite
if (NOT DEFINED ENV{LIB_FUZZING_ENGINE})
	set(COMPILATION_FLAGS_ "-g -ggdb2 -Wall -fsanitize=fuzzer,address,undefined")
else()
	set(COMPILATION_FLAGS_ "$ENV{LIB_FUZZING_ENGINE} $ENV{CXXFLAGS}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

string(REPLACE " " ";" COMPILATION_FLAGS ${COMPILATION_FLAGS_})

add_compile_options(${COMPILATION_FLAGS})
add_link_options(${COMPILATION_FLAGS})

add_library(xrp
  ../src/xrp/amount.c
  ../src/xrp/amount.h
  ../src/xrp/array.h
  ../src/xrp/fields.c
  ../src/xrp/fields.h
  ../src/xrp/field_sort.c
  ../src/xrp/field_sort.h
  ../src/xrp/flags.c
  ../src/xrp/flags.h
  ../src/xrp/fmt.c
  ../src/xrp/fmt.h
  ../src/xrp/general.c
  ../src/xrp/general.h
  ../src/xrp/number_helpers.c
  ../src/xrp/number_helpers.h
  ../src/xrp/percentage.c
  ../src/xrp/percentage.h
  ../src/xrp/readers.c
  ../src/xrp/readers.h
  ../src/xrp/ascii_strings.c
  ../src/xrp/ascii_strings.h
  ../src/xrp/time.c
  ../src/xrp/time.h
  ../src/xrp/transaction_types.h
  ../src/xrp/xrp_helpers.c
  ../src/xrp/xrp_helpers.h
  ../src/xrp/xrp_parse.c
  ../src/xrp/xrp_parse.h
)

include_directories(
    ../unit-tests/include
    ../src
    ../src/xrp
    ../src/apdu/messages
    ../src/apdu
)

add_executable(fuzz_tx
  src/fuzz_tx.c
  ../unit-tests/src/cx.c
  ../unit-tests/include/bolos_target.h
  ../unit-tests/include/cx.h
  ../unit-tests/include/os.h
)

target_link_libraries(fuzz_tx PUBLIC crypto ssl xrp)
