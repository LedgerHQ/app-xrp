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

void amount_formatter(field_t* field, field_value_t* dst) {
    uint64_t value = read_unsigned64(field->data.ptr);
    int error;

    if (field->length == XRP_AMOUNT_LEN) {
        error = format_xrp(value, dst);
    } else if (field->length == ISSUED_CURRENCY_LEN) {
        xrp_currency_t* currency = (xrp_currency_t*)&field->data.ptr[8];
        format_standard_currency(currency, dst->buf, sizeof(dst->buf));
        error = format_issued_currency(value, dst->buf, sizeof(dst->buf));
    } else {
        error = 1;
    }

    if (error) {
        strncpy(dst->buf, "Invalid amount!", sizeof(dst->buf));
    }
}

void currency_formatter(field_t* field, field_value_t* dst) {
    xrp_currency_t* currency = (xrp_currency_t*)field->data.ptr;
    format_non_standard_currency(currency, dst);
}

// An STNumber (serialized type 9) is a big-endian signed 64-bit mantissa
// followed by a big-endian signed 32-bit exponent. Its mantissa is not
// normalized to the 16 digits of an issued-currency amount (it can use 19), so
// parse_decimal_number() cannot print it.
#define NUMBER_MANTISSA_SIZE 8

// Writes c at offset if it fits with the terminating NUL (size is at least 1),
// and returns the next offset, or size once the buffer is full: an offset of
// size tells the caller that the number did not fit.
static size_t append_char(char* buf, size_t size, size_t offset, char c) {
    if (offset >= size - 1) {
        return size;
    }
    buf[offset] = c;
    return offset + 1;
}

static size_t append_str(char* buf, size_t size, size_t offset,
                         const char* str) {
    for (size_t i = 0; str[i] != '\x00'; i++) {
        offset = append_char(buf, size, offset, str[i]);
    }
    return offset;
}

// rippled keeps a Number in an XRP asset in drops (STAmount::operator
// Number), and a drop is 10^-6 XRP.
#define XRP_DROPS_EXPONENT (-6)

// Reads an STNumber as its sign, its magnitude without trailing zeros, and
// its exponent with those zeros added.
static void read_number(const uint8_t* number, bool* negative,
                        uint64_t* mantissa, int64_t* exponent) {
    const uint8_t* exponent_bytes = number + NUMBER_MANTISSA_SIZE;
    uint64_t raw_mantissa = read_unsigned64(number);
    uint32_t raw_exponent = ((uint32_t)exponent_bytes[0] << 24) |
                            ((uint32_t)exponent_bytes[1] << 16) |
                            ((uint32_t)exponent_bytes[2] << 8) |
                            (uint32_t)exponent_bytes[3];

    // Two's complement magnitude: the magnitude of INT64_MIN, 2^63, still fits.
    *negative = (raw_mantissa >> 63u) != 0;
    *mantissa = *negative ? ~raw_mantissa + 1 : raw_mantissa;

    // Sign-extend by arithmetic rather than a cast, into 64 bits, so that
    // a shift and stripping the trailing zeros cannot overflow at INT32_MAX.
    *exponent = (int64_t)raw_exponent;
    if (raw_exponent > INT32_MAX) {
        *exponent -= 0x100000000LL;
    }

    while (*mantissa != 0 && *mantissa % 10 == 0) {
        *mantissa /= 10;
        (*exponent)++;
    }
}

bool is_number_in_range(const uint8_t* number) {
    bool negative;
    uint64_t mantissa;
    int64_t exponent;
    read_number(number, &negative, &mantissa, &exponent);

    // Zero carries any exponent (rippled writes INT32_MIN). Otherwise, the
    // exponent range of an issued-currency amount, raised by the drops
    // shift so that a Number in an XRP asset stays within it too.
    return mantissa == 0 ||
           (exponent >= EXP_MIN - XRP_DROPS_EXPONENT && exponent <= EXP_MAX);
}

bool is_number_zero(const uint8_t* number) {
    return is_all_zeros(number, NUMBER_MANTISSA_SIZE);
}

int print_number(char* buf, size_t size, const uint8_t* number, int32_t shift) {
    if (size == 0) {
        return -1;
    }

    bool negative;
    uint64_t mantissa;
    int64_t exponent;
    read_number(number, &negative, &mantissa, &exponent);
    exponent += shift;

    size_t offset = 0;
    if (mantissa == 0) {
        offset = append_char(buf, size, offset, '0');
    } else {
        char digits[21] = {0};
        if (print_uint64_t(digits, sizeof(digits), mantissa) != 0) {
            buf[0] = '\x00';
            return -1;
        }
        int num_digits = (int)strlen(digits);

        if (negative) {
            offset = append_char(buf, size, offset, '-');
        }

        if (exponent < EXP_MIN || exponent > EXP_MAX) {
            // Outside the exponent range of an issued-currency amount, show
            // the exponent rather than a run of zeros too long to read.
            char magnitude[21] = {0};
            if (print_uint64_t(
                    magnitude, sizeof(magnitude),
                    (uint64_t)(exponent < 0 ? -exponent : exponent)) != 0) {
                buf[0] = '\x00';
                return -1;
            }
            offset = append_str(buf, size, offset, digits);
            offset = append_char(buf, size, offset, 'e');
            if (exponent < 0) {
                offset = append_char(buf, size, offset, '-');
            }
            offset = append_str(buf, size, offset, magnitude);
        } else {
            // Number of digits before the decimal point, zero or less for
            // 0.0...
            int point = num_digits + (int)exponent;
            if (point <= 0) {
                offset = append_str(buf, size, offset, "0.");
                for (int i = point; i < 0; i++) {
                    offset = append_char(buf, size, offset, '0');
                }
            }
            for (int i = 0; i < num_digits; i++) {
                if (i == point && point > 0) {
                    offset = append_char(buf, size, offset, '.');
                }
                offset = append_char(buf, size, offset, digits[i]);
            }
            for (int i = num_digits; i < point; i++) {
                offset = append_char(buf, size, offset, '0');
            }
        }
    }

    if (offset >= size) {
        buf[0] = '\x00';
        return -1;
    }
    buf[offset] = '\x00';
    return 0;
}

// A parsed Number is in range (is_number_in_range), and the longest one in
// range is a 100-character plain decimal: the replacement below is never
// shown, but a printer must not show a value it could not print whole.
void number_formatter(field_t* field, field_value_t* dst) {
    if (print_number(dst->buf, sizeof(dst->buf), field->data.ptr, 0) != 0) {
        strncpy(dst->buf, "Invalid number!", sizeof(dst->buf));
    }
}

void asset_number_formatter(field_t* field, xrp_currency_t* currency,
                            field_value_t* dst) {
    // The currency as an amount shows it: XRP, a standard code, or nothing
    // for a nonstandard code, which has a row of its own.
    format_standard_currency(currency, dst->buf, sizeof(dst->buf));
    size_t len = strnlen(dst->buf, sizeof(dst->buf));
    if (len > 0 && len + 1 < sizeof(dst->buf)) {
        dst->buf[len++] = ' ';
    }

    int32_t shift =
        is_all_zeros(currency->buf, XRP_CURRENCY_SIZE) ? XRP_DROPS_EXPONENT : 0;
    if (len >= sizeof(dst->buf) ||
        print_number(dst->buf + len, sizeof(dst->buf) - len, field->data.ptr,
                     shift) != 0) {
        strncpy(dst->buf, "Invalid number!", sizeof(dst->buf));
    }
}
