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

// A TransferRate or Quality is held in billionths over 1: 10000000 is 1 %
#define DENOMINATOR 10000000
// XLS-66 rates and fees are held in tenths of a basis point: 1000 is 1 %
#define TENTH_BPS_PER_PERCENT 1000

static bool is_tenth_bps_rate(field_t* field) {
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
    if (is_tenth_bps_rate(field)) {
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

// Prints value / per_percent as a percentage, per_percent being the power of
// ten that stands for 1 %: one decimal per zero of it, the trailing zeros
// then removed.
static void format_scaled_percentage(field_value_t* dst, uint32_t value,
                                     uint32_t per_percent) {
    uint32_t decimal_part = value % per_percent;

    snprintf(dst->buf, sizeof(dst->buf), "%u.", value / per_percent);
    size_t len = strlen(dst->buf);
    for (uint32_t unit = per_percent / 10; unit > 0; unit /= 10) {
        if (len < sizeof(dst->buf) - 1) {
            dst->buf[len++] = (char)('0' + decimal_part / unit % 10);
        }
    }
    dst->buf[len] = '\x00';
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
        format_scaled_percentage(dst, value - 1000000000, DENOMINATOR);
    }
}

static void format_quality(field_value_t* dst, uint32_t value) {
    if (value == 0) {
        strncpy(dst->buf, "100 %", sizeof(dst->buf));
    } else {
        format_scaled_percentage(dst, value, DENOMINATOR);
    }
}

void format_percentage(field_t* field, field_value_t* dst) {
    if (is_tenth_bps_rate(field)) {
        uint32_t rate =
            field->data_type == STI_UINT16 ? field->data.u16 : field->data.u32;
        format_scaled_percentage(dst, rate, TENTH_BPS_PER_PERCENT);
        return;
    }

    uint32_t value = field->data.u32;

    if (field->id == XRP_UINT32_TRANSFER_RATE) {
        format_transfer_rate(dst, value);
    } else {
        format_quality(dst, value);
    }
}
