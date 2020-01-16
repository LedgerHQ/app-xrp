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

#include <os_io_seproxyhal.h>
#include <string.h>
#include "readers.h"

uint64_t readUnsigned(const uint8_t *src, uint8_t numBytes) {
    uint64_t value = 0;
    for (uint8_t i = 0; i < numBytes; ++i) {
        value |= (uint64_t) src[i] << (numBytes * 8u - i * 8u - 8u);
    }

    return value;
}

void readHex(char *dst, uint8_t *src, uint16_t dataLength) {
    for (uint16_t i = 0; i < dataLength; i++) {
        SPRINTF(dst + 2 * i, "%02X", src[i]);
    }
}

void readString(char *dst, uint8_t *src, uint8_t dataLength) {
    memcpy(dst, src, dataLength);
}

uint16_t readUnsigned8(const uint8_t *src) {
    return (int8_t) readUnsigned(src, 1);
}

uint16_t readUnsigned16(const uint8_t *src) {
    return (int16_t) readUnsigned(src, 2);
}

uint32_t readUnsigned32(const uint8_t *src) {
    return (uint32_t) readUnsigned(src, 4);
}

uint64_t readUnsigned64(const uint8_t *src) {
    return readUnsigned(src, 8);
}
