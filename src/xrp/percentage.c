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

#include "percentage.h"

#include <stdio.h>
#include <string.h>

#include "fmt.h"
#include "limitations.h"
#include "readers.h"

#define DENOMINATOR 10000000

// Lending-protocol rates (XLS-66) are in 1/10 basis point: 100000 = 100 %.
static bool is_lending_rate(field_t* field) {
    if (field->data_type == STI_UINT16) {
        return field->id == XRP_UINT16_MANAGEMENT_FEE_RATE;
    }

    if (field->data_type == STI_UINT32) {
        switch (field->id) {
            case XRP_UINT32_COVER_RATE_MINIMUM:
            case XRP_UINT32_COVER_RATE_LIQUIDATION:
            case XRP_UINT32_OVERPAYMENT_FEE:
            case XRP_UINT32_INTEREST_RATE:
            case XRP_UINT32_LATE_INTEREST_RATE:
            case XRP_UINT32_CLOSE_INTEREST_RATE:
            case XRP_UINT32_OVERPAYMENT_INTEREST_RATE:
                return true;
            default:
                return false;
        }
    }

    return false;
}

bool is_percentage(field_t* field) {
    if (is_lending_rate(field)) {
        return true;
    }

    if (field->data_type == STI_UINT32) {
        switch (field->id) {
            case XRP_UINT32_TRANSFER_RATE:
            case XRP_UINT32_QUALITY_IN:
            case XRP_UINT32_QUALITY_OUT:
                return true;
            default:
                return false;
        }
    } else {
        return false;
    }
}

void remove_redundant_decimals(field_value_t* dst) {
    size_t end = strlen(dst->buf);
    while (end > 0) {
        char c = dst->buf[end];
        if (c == '0' || c == '\0') {
            // Redundant character, remove and continue searching
            end--;
            continue;
        } else if (c == '.') {
            // We have reached the end of the decimals, break at next char
            end--;
            break;
        } else {
            // We have reached a non-zero decimal, break at current char
            break;
        }
    }

    dst->buf[end + 1] = '\0';
}

static void format_percentage_internal(field_value_t* dst, uint32_t value) {
    unsigned int decimal_part = value % DENOMINATOR;
    unsigned int integer_part = (value - decimal_part) / DENOMINATOR;

    snprintf(dst->buf, sizeof(dst->buf), "%u.%07u", integer_part, decimal_part);
    remove_redundant_decimals(dst);

    size_t total_length = strlen(dst->buf);
    strncpy(dst->buf + total_length, " %", sizeof(dst->buf) - total_length);
}

static void format_transfer_rate(field_value_t* dst, uint32_t value) {
    if (value == 0) {
        strncpy(dst->buf, "0 %", sizeof(dst->buf));
    } else if (value < 1000000000) {
        strncpy(dst->buf, "Invalid value", sizeof(dst->buf));
    } else {
        format_percentage_internal(dst, value - 1000000000);
    }
}

static void format_quality(field_value_t* dst, uint32_t value) {
    if (value == 0) {
        strncpy(dst->buf, "100 %", sizeof(dst->buf));
    } else {
        format_percentage_internal(dst, value);
    }
}

// 1/10 basis point to percent: 500 -> "0.5 %", 100000 -> "100 %".
static void format_lending_rate(field_value_t* dst, uint32_t value) {
    snprintf(dst->buf, sizeof(dst->buf), "%u.%03u", value / 1000,
             value % 1000);
    remove_redundant_decimals(dst);

    size_t total_length = strlen(dst->buf);
    strncpy(dst->buf + total_length, " %", sizeof(dst->buf) - total_length);
}

void format_percentage(field_t* field, field_value_t* dst) {
    if (is_lending_rate(field)) {
        format_lending_rate(dst, field->data_type == STI_UINT16
                                     ? field->data.u16
                                     : field->data.u32);
        return;
    }

    uint32_t value = field->data.u32;

    if (field->id == XRP_UINT32_TRANSFER_RATE) {
        format_transfer_rate(dst, value);
    } else {
        format_quality(dst, value);
    }
}
