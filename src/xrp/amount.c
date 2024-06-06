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
#include <stdbool.h>

#include "amount.h"
#include "fmt.h"
#include "xrp_helpers.h"
#include "readers.h"
#include "number_helpers.h"
#include "limitations.h"
#include "ascii_strings.h"

#define EXP_MIN      -96
#define EXP_MAX      80
#define MANTISSA_MIN 1000000000000000
#define MANTISSA_MAX 9999999999999999

static void normalize(uint64_t *mantissa_param, int16_t *exponent_param) {
    while (*mantissa_param > 0 && *mantissa_param % 10 == 0) {
        *mantissa_param = *mantissa_param / 10;
        (*exponent_param)++;
    }
}

static int print_uint64_t(char *dst, uint16_t len, uint64_t value) {
    uint16_t num_digits = 0, i;
    uint64_t base = 1;

    while (base <= value) {
        base *= 10;
        num_digits++;
    }

    if (num_digits > len - 1) {
        return -1;
    }

    base /= 10;
    for (i = 0; i < num_digits; i++) {
        dst[i] = int_to_number_char((value / base) % 10);
        base /= 10;
    }

    dst[i] = '\x00';

    return 0;
}

static int parse_decimal_number(char *dst,
                                size_t max_len,
                                uint8_t sign,
                                int16_t exponent,
                                uint64_t mantissa) {
    if (max_len < 100) {
        return -1;
    }

    // 0. Abort early if number matches special case for zero
    if (sign == 0 && exponent == 0 && mantissa == 0) {
        dst[0] = '0';
        return 0;
    }

    if (exponent < EXP_MIN || exponent > EXP_MAX) {
        return -1;
    }

    if (mantissa < MANTISSA_MIN || mantissa > MANTISSA_MAX) {
        return -1;
    }

    // 1. Add leading minus sign if number is negative
    if (sign == 0) {
        dst[0] = '-';

        dst++;
        max_len--;
    }

    // 2. Normalize mantissa by removing redundant trailing zeros
    normalize(&mantissa, &exponent);

    // 3. Print the entire mantissa to our buffer
    if (print_uint64_t(dst, max_len, mantissa) != 0) {
        return -1;
    }

    // 4. Store the length of our normalized mantissa
    size_t len = strlen(dst);

    // 5. Calculate the position of the decimal point relative to dst
    int16_t decimal_pos = len + exponent;

    if (exponent >= 0) {
        // Exponent is positive, "multiply" the mantissa (decimalPos not needed)
        memset(dst + len, '0', exponent);
    } else if (decimal_pos > 0) {
        // Decimal position is within our bounds, make room for it and add it
        memmove(dst + decimal_pos + 1, dst + decimal_pos, len);
        dst[decimal_pos] = '.';
    } else {
        // Decimal position is outside our bounds, move the mantissa to make
        // sure that the decimal point can fit within the new bounds
        memmove(dst - decimal_pos + 2, dst, len);
        memset(dst, '0', -decimal_pos + 2);
        dst[1] = '.';
    }

    return 0;
}

static int format_xrp(uint64_t amount, field_value_t *dst) {
    if (!(amount & 0x4000000000000000)) {
        return -1;
    }

    amount ^= 0x4000000000000000;
    if (xrp_print_amount(amount, dst->buf, sizeof(dst->buf)) != 0) {
        return -1;
    }

    return 0;
}

bool is_all_zeros(const uint8_t *data, uint8_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (data[i] != 0) {
            return false;
        }
    }

    return true;
}

static bool has_non_standard_currency_internal(const uint8_t *currency_data) {
    return currency_data[0] != 0x00;
}

bool has_non_standard_currency(field_t *field) {
    return has_non_standard_currency_internal(&field->data.ptr[8]);
}

static void format_standard_currency(uint8_t *currency_data, char *buf, size_t size) {
    if (has_non_standard_currency_internal(currency_data)) {
    } else if (is_all_zeros(currency_data, 20)) {
        // Special case for XRP currency
        strncpy(buf, "XRP", size);
    } else {
        // Standard currency code
        memcpy(buf, &currency_data[12], 3);
    }
}

static void format_non_standard_currency(xrp_currency_t *currency, field_value_t *dst) {
    if (has_non_standard_currency_internal(currency->buf)) {
        // Nonstandard currency code
        bool contains_only_ascii = is_purely_ascii(currency->buf, sizeof(currency->buf), true);
        if (contains_only_ascii && currency->buf[sizeof(currency->buf) - 1] == '\x00' &&
            strstr((char *) currency->buf, "XRP")) {
            memcpy(dst->buf, currency->buf, 20);
        } else {
            read_hex(dst->buf, sizeof(dst->buf), currency->buf, sizeof(currency->buf));
        }
    } else if (is_all_zeros(currency->buf, sizeof(currency->buf))) {
        // Special case for XRP currency
        strncpy(dst->buf, "XRP", sizeof(dst->buf));
    } else {
        // Standard currency code
        memcpy(dst->buf, &currency->buf[12], 3);
    }
}

static int format_issued_currency(uint64_t value, char *buf, size_t size) {
    uint8_t sign = (uint8_t) ((value >> 62u) & 0x01u);
    int16_t exponent = (int16_t) (((value >> 54u) & 0xFFu) - 97);
    uint64_t mantissa = value & 0x3FFFFFFFFFFFFFu;
    size_t len = strlen(buf);
    char *p;

    p = buf + len;
    size -= len;

    if (size == 0) {
        return -1;
    }

    if (len > 0) {
        // Only add space if a currency was printed
        *p++ = ' ';
        size--;
    }

    if (value << 1u == 0) {
        // Special case for the value zero
        *p++ = '0';
        size--;
        return 0;
    }

    return parse_decimal_number(p, size, sign, exponent, mantissa);
}

void amount_formatter(field_t *field, field_value_t *dst) {
    uint64_t value = read_unsigned64(field->data.ptr);
    int error;

    if (field->length == XRP_AMOUNT_LEN) {
        error = format_xrp(value, dst);
    } else if (field->length == ISSUED_CURRENCY_LEN) {
        format_standard_currency(&field->data.ptr[8], dst->buf, sizeof(dst->buf));
        error = format_issued_currency(value, dst->buf, sizeof(dst->buf));
    } else {
        error = 1;
    }

    if (error) {
        strncpy(dst->buf, "Invalid amount!", sizeof(dst->buf));
    }
}

void currency_formatter(field_t *field, field_value_t *dst) {
    xrp_currency_t *currency = (xrp_currency_t *) field->data.ptr;
    format_non_standard_currency(currency, dst);
}
