cmake_minimum_required(VERSION 3.10)
project(test_parser C)

include(CTest)

set(CMAKE_C_STANDARD 11)

# required for fuzzing
set(CMAKE_C_COMPILER /usr/bin/clang)

enable_testing()

include_directories(
  include
  ../src
  ../src/xrp
  ../src/apdu/messages
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
  ../src/swap/handle_check_address.h
  ../src/swap/swap_utils.c
  ../src/swap/swap_utils.h
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

add_executable(fuzz_tx
  src/fuzz_tx.c
  src/cx.c
  include/bolos_target.h
  include/cx.h
  include/os.h
)

target_compile_options(fuzz_tx PRIVATE -Wall -fsanitize=fuzzer,address -g -ggdb2)
target_link_libraries(fuzz_tx PRIVATE cmocka crypto ssl xrp -fsanitize=fuzzer,address)
target_link_libraries(test_printers PRIVATE cmocka crypto ssl xrp)
target_link_libraries(test_swap PRIVATE cmocka crypto ssl xrp)
target_link_libraries(test_tx PRIVATE cmocka crypto ssl xrp)

add_test(test_printers test_printers)
add_test(test_swap test_swap)
add_test(test_tx test_tx)
