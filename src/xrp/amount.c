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

#include "amount.h"

#include <stdbool.h>
#include <string.h>

#include "ascii_strings.h"
#include "fields.h"
#include "fmt.h"
#include "limitations.h"
#include "number_helpers.h"
#include "readers.h"
#include "xrp_helpers.h"

#define EXP_MIN -96
#define EXP_MAX 80
#define MANTISSA_MIN 1000000000000000
#define MANTISSA_MAX 9999999999999999

static void normalize(uint64_t* mantissa_param, int16_t* exponent_param) {
    while (*mantissa_param > 0 && *mantissa_param % 10 == 0) {
        *mantissa_param = *mantissa_param / 10;
        (*exponent_param)++;
    }
}

static int print_uint64_t(char* dst, uint16_t len, uint64_t value) {
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

static int parse_decimal_number(char* dst, size_t max_len, uint8_t sign,
                                int16_t exponent, uint64_t mantissa) {
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

static int format_xrp(uint64_t amount, field_value_t* dst) {
    if (!(amount & 0x4000000000000000)) {
        return -1;
    }

    amount ^= 0x4000000000000000;
    if (xrp_print_amount(amount, dst->buf, sizeof(dst->buf)) != 0) {
        return -1;
    }

    return 0;
}

bool is_all_zeros(const uint8_t* data, uint8_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (data[i] != 0) {
            return false;
        }
    }

    return true;
}

#define XRP_CURRENCY_TICKER_START 12
#define XRP_CURRENCY_TICKER_END 14

// https://xrpl.org/docs/references/protocol/data-types/currency-formats#standard-currency-codes
static bool is_valid_ticker_char(uint8_t c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '?' || c == '!' || c == '@' ||
           c == '#' || c == '$' || c == '%' || c == '^' || c == '&' ||
           c == '*' || c == '<' || c == '>' || c == '(' || c == ')' ||
           c == '{' || c == '}' || c == '[' || c == ']' || c == '|';
}

static bool is_standard_currency_code(const uint8_t* currency_data) {
    for (size_t i = 0; i < XRP_CURRENCY_SIZE; i++) {
        if (i >= XRP_CURRENCY_TICKER_START && i <= XRP_CURRENCY_TICKER_END) {
            if (!is_valid_ticker_char(currency_data[i])) {
                return false;
            }
        } else if (currency_data[i] != 0x00) {
            return false;
        }
    }
    // All-uppercase "XRP" is reserved for native XRP (all-zero 20 bytes).
    if (currency_data[XRP_CURRENCY_TICKER_START] == 'X' &&
        currency_data[XRP_CURRENCY_TICKER_START + 1] == 'R' &&
        currency_data[XRP_CURRENCY_TICKER_END] == 'P') {
        return false;
    }
    return true;
}

// Reject currencies that have standard-code structure (all non-ticker bytes
// zero, not all-zeros) but use the reserved "XRP" ticker. Any other form —
// native XRP, valid standard codes, or nonstandard codes — is accepted.
bool is_valid_currency(const uint8_t* currency_data) {
    if (is_all_zeros(currency_data, XRP_CURRENCY_SIZE)) {
        return true;
    }
    bool standard_structure = true;
    for (size_t i = 0; i < XRP_CURRENCY_SIZE; i++) {
        if (i >= XRP_CURRENCY_TICKER_START && i <= XRP_CURRENCY_TICKER_END) {
            continue;
        }
        if (currency_data[i] != 0x00) {
            standard_structure = false;
            break;
        }
    }
    if (standard_structure && currency_data[XRP_CURRENCY_TICKER_START] == 'X' &&
        currency_data[XRP_CURRENCY_TICKER_START + 1] == 'R' &&
        currency_data[XRP_CURRENCY_TICKER_END] == 'P') {
        return false;
    }
    return true;
}

static bool has_non_standard_currency_internal(const uint8_t* currency_data) {
    return !is_standard_currency_code(currency_data) &&
           !is_all_zeros(currency_data, XRP_CURRENCY_SIZE);
}

bool has_non_standard_currency(field_t* field) {
    return has_non_standard_currency_internal(&field->data.ptr[8]);
}

static void format_standard_currency(xrp_currency_t* currency, char* buf,
                                     size_t size) {
    if (has_non_standard_currency_internal(currency->buf)) {
    } else if (is_all_zeros(currency->buf, XRP_CURRENCY_SIZE)) {
        // Special case for XRP currency
        strncpy(buf, "XRP", size);
    } else {
        // Standard currency code
        memcpy(buf, &currency->buf[12], 3);
    }
}

static void format_non_standard_currency(xrp_currency_t* currency,
                                         field_value_t* dst) {
    if (has_non_standard_currency_internal(currency->buf)) {
        // Nonstandard currency code
        bool contains_only_ascii =
            is_purely_ascii(currency->buf, sizeof(currency->buf), true);
        if (contains_only_ascii &&
            currency->buf[sizeof(currency->buf) - 1] == '\x00' &&
            !strstr((char*)currency->buf, "XRP")) {
            memcpy(dst->buf, currency->buf, XRP_CURRENCY_SIZE);
        } else {
            read_hex(dst->buf, sizeof(dst->buf), currency->buf,
                     sizeof(currency->buf));
        }
    } else if (is_all_zeros(currency->buf, sizeof(currency->buf))) {
        // Special case for XRP currency
        strncpy(dst->buf, "XRP", sizeof(dst->buf));
    } else {
        // Standard currency code
        memcpy(dst->buf, &currency->buf[12], 3);
    }
}

static int format_issued_currency(uint64_t value, char* buf, size_t size) {
    uint8_t sign = (uint8_t)((value >> 62u) & 0x01u);
    int16_t exponent = (int16_t)(((value >> 54u) & 0xFFu) - 97);
    uint64_t mantissa = value & 0x3FFFFFFFFFFFFFu;
    size_t len = strlen(buf);
    char* p;

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

// MPT amount: flag byte (0x20 = MPT, 0x40 = positive), 8-byte unsigned
// value, then the 24-byte MPTokenIssuanceID that read_amount() exposes as
// its own "MPT Issuance ID" field. Printed as "MPT <value>" in the token's
// own smallest unit, since the AssetScale lives on the issuance, not here.
static int format_mpt(field_t* field, field_value_t* dst) {
    if ((field->data.ptr[0] & 0x40u) == 0) {
        return -1;
    }

    uint64_t value = read_unsigned64(field->data.ptr + 1);
    if (value >> 63u) {
        return -1;
    }

    strncpy(dst->buf, "MPT ", sizeof(dst->buf));
    return print_uint64_t(dst->buf + 4, sizeof(dst->buf) - 4, value);
}

void amount_formatter(field_t* field, field_value_t* dst) {
    int error;

    if (field->length == XRP_AMOUNT_LEN) {
        error = format_xrp(read_unsigned64(field->data.ptr), dst);
    } else if (field->length == ISSUED_CURRENCY_LEN) {
        uint64_t value = read_unsigned64(field->data.ptr);
        xrp_currency_t* currency = (xrp_currency_t*)&field->data.ptr[8];
        format_standard_currency(currency, dst->buf, sizeof(dst->buf));
        error = format_issued_currency(value, dst->buf, sizeof(dst->buf));
    } else if (field->length == MPT_AMOUNT_LEN) {
        error = format_mpt(field, dst);
    } else {
        error = 1;
    }

    if (error) {
        strncpy(dst->buf, "Invalid amount!", sizeof(dst->buf));
    }
}

void currency_formatter(field_t* field, field_value_t* dst) {
    if (field->data_type == STI_ISSUE && field->length == XRP_MPT_ISSUE_SIZE) {
        // Print the MPTokenIssuanceID the way explorers and xrpl.js do:
        // 4-byte sequence followed by the 20-byte issuer. The codecs write
        // the sequence byte-swapped in an Issue, so swap it back here.
        uint8_t sequence[4] = {field->data.ptr[43], field->data.ptr[42],
                               field->data.ptr[41], field->data.ptr[40]};
        strncpy(dst->buf, "MPT ", sizeof(dst->buf));
        read_hex(dst->buf + 4, sizeof(dst->buf) - 4, sequence,
                 sizeof(sequence));
        read_hex(dst->buf + 12, sizeof(dst->buf) - 12, field->data.ptr,
                 XRP_ACCOUNT_SIZE);
        return;
    }

    xrp_currency_t* currency = (xrp_currency_t*)field->data.ptr;
    format_non_standard_currency(currency, dst);
}

void uint64_formatter(field_t* field, field_value_t* dst) {
    uint64_t value = read_unsigned64(field->data.ptr);
    if (print_uint64_t(dst->buf, sizeof(dst->buf), value) != 0) {
        strncpy(dst->buf, "[ERROR DURING FORMATTING]", sizeof(dst->buf));
    }
}

// Widest exponent printed in plain decimal; beyond that the value is
// shown as <digits>e<exponent> so it always fits the buffer.
#define NUMBER_PLAIN_EXP_MAX 20
#define NUMBER_PLAIN_EXP_MIN -40

// STNumber: int64 mantissa then int32 exponent, both big-endian, value =
// mantissa * 10^exponent. rippled normalizes the mantissa to 17-19 digits,
// so "1000" arrives as 1000000000000000000e-15. Zero is mantissa 0 with a
// sentinel exponent. No currency is printed: the unit is the vault asset,
// which is not part of the transaction.
void number_formatter(field_t* field, field_value_t* dst) {
    uint64_t raw = read_unsigned64(field->data.ptr);
    int32_t exponent = (int32_t)(((uint32_t)field->data.ptr[8] << 24) |
                                 ((uint32_t)field->data.ptr[9] << 16) |
                                 ((uint32_t)field->data.ptr[10] << 8) |
                                 (uint32_t)field->data.ptr[11]);

    if (raw == 0) {
        strncpy(dst->buf, "0", sizeof(dst->buf));
        return;
    }

    bool negative = (raw >> 63u) != 0;
    uint64_t mantissa = negative ? (~raw + 1u) : raw;

    while (mantissa % 10 == 0) {
        mantissa /= 10;
        exponent++;
    }

    char digits[21];
    if (print_uint64_t(digits, sizeof(digits), mantissa) != 0) {
        strncpy(dst->buf, "[ERROR DURING FORMATTING]", sizeof(dst->buf));
        return;
    }
    size_t len = strlen(digits);
    char* p = dst->buf;
    size_t room = sizeof(dst->buf);

    if (negative) {
        *p++ = '-';
        room--;
    }

    if (exponent > NUMBER_PLAIN_EXP_MAX || exponent < NUMBER_PLAIN_EXP_MIN) {
        snprintf(p, room, "%se%d", digits, exponent);
    } else if (exponent >= 0) {
        memcpy(p, digits, len);
        memset(p + len, '0', exponent);
        p[len + exponent] = '\0';
    } else if ((int32_t)len + exponent > 0) {
        size_t decimal_pos = len + exponent;
        memcpy(p, digits, decimal_pos);
        p[decimal_pos] = '.';
        memcpy(p + decimal_pos + 1, digits + decimal_pos, len - decimal_pos);
        p[len + 1] = '\0';
    } else {
        size_t zeros = (size_t)(-exponent) - len;
        p[0] = '0';
        p[1] = '.';
        memset(p + 2, '0', zeros);
        memcpy(p + 2 + zeros, digits, len);
        p[2 + zeros + len] = '\0';
    }
}
