cmake_minimum_required(VERSION 3.10)
project(test_parser C)

include(CTest)

set(CMAKE_C_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 --coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -lgcov")

enable_testing()

include_directories(
  include
  ../src
  ../src/xrp
  ../src/apdu/messages
  ../src/apdu
)

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

add_executable(test_printers
  src/test_printers.c
  src/cx.c
  include/bolos_target.h
  include/cx.h
  include/os.h
)

add_executable(test_swap
  src/test_swap.c
  src/cx.c
  include/bolos_target.h
  include/cx.h
  include/os.h
  )

add_executable(test_tx
  src/test_tx.c
  src/cx.c
  include/bolos_target.h
  include/cx.h
  include/os.h
)

add_executable(test_signer_list_set
  src/test_signer_list_set.c
  src/cx.c
  include/bolos_target.h
  include/cx.h
  include/os.h
)

target_link_libraries(test_printers PRIVATE cmocka crypto ssl xrp)
target_link_libraries(test_swap PRIVATE cmocka crypto ssl xrp)
target_link_libraries(test_tx PRIVATE cmocka crypto ssl xrp)
target_link_libraries(test_signer_list_set PRIVATE cmocka crypto ssl xrp)

add_test(test_printers test_printers)
add_test(test_swap test_swap)
add_test(test_tx test_tx)
add_test(test_signer_list_set test_signer_list_set)
