/*******************************************************************************
 *   XRP Wallet
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

#include <assert.h>
#include <string.h>

#include "numberHelpers.h"
#include "os.h"

// Needed to make app compile with Nano X SDK
#define __assert_fail os_sched_exit(1);

void normalize(uint64_t *mantissaParam, int16_t *exponentParam);

void parseDecimalNumber(char *dst,
                        uint16_t maxLen,
                        uint8_t sign,
                        int16_t exponent,
                        uint64_t mantissa) {
    assert(maxLen >= 100);
    assert(exponent >= EXP_MIN);
    assert(exponent <= EXP_MAX);
    assert(mantissa >= MANTISSA_MIN);
    assert(mantissa <= MANTISSA_MAX);

    // 0. Abort early if number matches special case for zero
    if (sign == 0 && exponent == 0 && mantissa == 0) {
        dst[0] = '0';
        return;
    }

    // 1. Add leading minus sign if number is negative
    if (sign == 0) {
        dst[0] = '-';

        dst++;
        maxLen--;
    }

    // 2. Normalize mantissa by removing redundant trailing zeros
    normalize(&mantissa, &exponent);

    // 3. Print the entire mantissa to our buffer
    print_uint64_t(dst, maxLen, mantissa);

    // 4. Store the length of our normalized mantissa
    uint16_t len = strlen(dst);

    // 5. Calculate the position of the decimal point relative to dst
    int16_t decimalPos = len + exponent;

    if (exponent >= 0) {
        // Exponent is positive, "multiply" the mantissa (decimalPos not needed)
        memset(dst + len, '0', exponent);
    } else if (decimalPos > 0) {
        // Decimal position is within our bounds, make room for it and add it
        os_memmove(dst + decimalPos + 1, dst + decimalPos, len);
        dst[decimalPos] = '.';
    } else {
        // Decimal position is outside our bounds, move the mantissa to make
        // sure that the decimal point can fit within the new bounds
        os_memmove(dst - decimalPos + 2, dst, len);
        memset(dst, '0', -decimalPos + 2);
        dst[1] = '.';
    }
}

void normalize(uint64_t *mantissaParam, int16_t *exponentParam) {
    while (*mantissaParam > 0 && *mantissaParam % 10 == 0) {
        *mantissaParam = *mantissaParam / 10;
        (*exponentParam)++;
    }
}

char intToNumberChar(uint64_t value) {
    if (value > 9) {
        return '?';
    }

    return (char) ('0' + value);
}

void print_uint64_t(char *dst, uint16_t len, uint64_t value) {
    uint16_t numDigits = 0, i;
    uint64_t base = 1;
    while (base <= value) {
        base *= 10;
        numDigits++;
    }
    if (numDigits > len - 1) {
        THROW(NOT_ENOUGH_SPACE);
    }
    base /= 10;
    for (i = 0; i < numDigits; i++) {
        dst[i] = intToNumberChar((value / base) % 10);
        base /= 10;
    }
    dst[i] = '\0';
}
