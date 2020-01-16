/*******************************************************************************
*   XRP Wallet
*   (c) 2017 Ledger
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

#include "xrpBase58.h"
#include "../limitations.h"

static const char BASE58ALPHABET[] = {
    'r', 'p', 's', 'h', 'n', 'a', 'f', '3', '9', 'w', 'B', 'U', 'D', 'N', 'E',
    'G', 'H', 'J', 'K', 'L', 'M', '4', 'P', 'Q', 'R', 'S', 'T', '7', 'V', 'W',
    'X', 'Y', 'Z', '2', 'b', 'c', 'd', 'e', 'C', 'g', '6', '5', 'j', 'k', 'm',
    '8', 'o', 'F', 'q', 'i', '1', 't', 'u', 'v', 'A', 'x', 'y', 'z'
};

unsigned short xrp_encode_base58(const unsigned char WIDE *in, unsigned char length,
                                char *out, unsigned short maxoutlen) {
    unsigned char buffer[MAX_ENC_INPUT_SIZE * 138 / 100 + 1] = {0};
    size_t i = 0, j;
    size_t startAt, stopAt;
    size_t zeroCount = 0;
    size_t outputSize;
    unsigned short outlen = maxoutlen;

    if (length > MAX_ENC_INPUT_SIZE) {
        THROW(EXCEPTION_OVERFLOW);
    }

    while ((zeroCount < length) && (in[zeroCount] == 0)) {
        ++zeroCount;
    }

    outputSize = (length - zeroCount) * 138 / 100 + 1;
    stopAt = outputSize - 1;
    for (startAt = zeroCount; startAt < length; startAt++) {
        int carry = in[startAt];
        for (j = outputSize - 1; (int)j >= 0; j--) {
            carry += 256 * buffer[j];
            buffer[j] = carry % 58;
            carry /= 58;

            if (j <= stopAt - 1 && carry == 0) {
                break;
            }
        }
        stopAt = j;
    }

    j = 0;
    while (j < outputSize && buffer[j] == 0) {
        j += 1;
    }

    if (outlen < zeroCount + outputSize - j) {
        THROW(EXCEPTION_OVERFLOW);
    }

    os_memset(out, BASE58ALPHABET[0], zeroCount);

    i = zeroCount;
    while (j < outputSize) {
        out[i++] = BASE58ALPHABET[buffer[j++]];
    }
    outlen = i;

    return outlen;
}
