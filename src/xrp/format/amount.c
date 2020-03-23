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
#include "format.h"
#include "../xrpHelpers.h"
#include "readers.h"
#include "../parse/numberHelpers.h"
#include "../../limitations.h"
#include "strings.h"
#include <string.h>
#include <stdbool.h>

#define XRP_AMOUNT_LEN 8
#define ISSUED_CURRENCY_LEN 48

void formatXRP(field_t* field, char *dst) {
    uint64_t value = readUnsigned64(field->data);

    value -= (uint64_t)0x4000000000000000;
    xrp_print_amount(value, dst, MAX_FIELD_LEN);
}

bool isAllZeros(const uint8_t *data, uint8_t length) {
    for (uint8_t i = 0; i < length; ++i) {
        if (data[i] != 0) {
            return false;
        }
    }

    return true;
}

bool hasNonStandardCurrencyInternal(const uint8_t *currencyData) {
    return currencyData[0] != 0x00;
}

bool hasNonStandardCurrency(field_t* field) {
    return hasNonStandardCurrencyInternal(&field->data[8]);
}

void formatCurrency(uint8_t *currencyData, char *dst, bool omitNonStandard) {
    if (hasNonStandardCurrencyInternal(currencyData)) {
        if (!omitNonStandard) {
            // Nonstandard currency code
            bool containsOnlyAscii = isPurelyAscii(currencyData, 20, true);
            bool containsSubstringXRP = strstr(currencyData, "XRP");

            if (containsOnlyAscii && !containsSubstringXRP) {
                readString(dst, currencyData, 20);
            } else {
                readHex(dst, currencyData, 20);
            }
        }
    } else if (isAllZeros(currencyData, 20)) {
        // Special case for XRP currency
        strcpy(dst, "XRP");
    } else {
        // Standard currency code
        readString(dst, &currencyData[12], 3);
    }
}

void formatIssuedCurrency(field_t* field, char *dst) {
    uint64_t value = readUnsigned64(field->data);

    uint8_t sign = (uint8_t) ((value >> 62u) & 0x01u);
    int16_t exponent = (int16_t) (((value >> 54u) & 0xFFu) - 97);
    uint64_t mantissa = value & 0x3FFFFFFFFFFFFFu;

    formatCurrency(&field->data[8], dst, true);

    uint16_t textPos = strlen(dst);
    if (textPos > 0) {
        // Only add space if a currency was printed
        dst[textPos++] = ' ';
    }

    if (value << 1u == 0) {
        // Special case for the value zero
        dst[textPos] = '0';
        return;
    }

    if (exponent < EXP_MIN || exponent > EXP_MAX) {
        SNPRINTF(dst, "Invalid exponent!");
        return;
    }

    if (mantissa < MANTISSA_MIN || mantissa > MANTISSA_MAX) {
        SNPRINTF(dst, "Invalid mantissa!");
        return;
    }

    parseDecimalNumber(dst + textPos, MAX_FIELD_LEN - textPos, sign, exponent, mantissa);
}

void amountFormatter(field_t* field, char *dst) {
    switch (field->length) {
        case XRP_AMOUNT_LEN:
            formatXRP(field, dst);
            break;
        case ISSUED_CURRENCY_LEN:
            formatIssuedCurrency(field, dst);
            break;
        default:
            SNPRINTF(dst, "Invalid amount!");
    }
}

void currencyFormatter(field_t* field, char *dst) {
    formatCurrency(field->data, dst, false);
}
