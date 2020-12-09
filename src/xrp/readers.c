/*******************************************************************************
 *   XRP Wallet
 *   (c) 2017 Ledger
 *   (c) 2020 Towo Labs
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#include <string.h>

#include "readers.h"

uint64_t read_unsigned64(const uint8_t *src) {
    uint64_t value = 0;
    const size_t num_bytes = 8;
    for (uint8_t i = 0; i < num_bytes; ++i) {
        value |= (uint64_t) src[i] << (num_bytes * 8u - i * 8u - 8u);
    }

    return value;
}

static char hex(uint8_t n) {
    return n >= 10 ? 'a' + (n - 10) : '0' + n;
}

bool read_hex(char *dst, size_t dst_size, uint8_t *src, size_t src_size) {
    size_t i;

    for (i = 0; i < src_size && i * 2 + 1 < dst_size; i++) {
        dst[i * 2 + 0] = hex(src[i] >> 4);
        dst[i * 2 + 1] = hex(src[i] & 0xf);
    }

    return i == src_size;
}
