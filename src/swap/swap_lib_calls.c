#include <string.h>

#include "swap_lib_calls.h"

bool swap_str_to_u64(const uint8_t* src, size_t length, uint64_t* result) {
    const size_t num_bytes = 8;
    uint8_t buffer[8];

    if (length > sizeof(buffer)) {
        return false;
    }

    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer + sizeof(buffer) - length, src, length);

    uint64_t value = 0;
    for (uint8_t i = 0; i < num_bytes; ++i) {
        value |= (uint64_t) buffer[i] << (num_bytes * 8u - i * 8u - 8u);
    }

    *result = value;

    return true;
}
