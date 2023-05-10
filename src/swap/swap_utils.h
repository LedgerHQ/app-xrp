#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

bool swap_str_to_u64(const uint8_t* src, size_t length, uint64_t* result);
