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
#include "readers.h"
#include "format.h"
#include "../../limitations.h"
#include <string.h>

#define DENOMINATOR 10000000

bool isPercentage(field_t* field) {
    if (field->dataType == STI_UINT32) {
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

void removeRedundantDecimals(char *dst) {
    size_t end = strlen(dst);
    while (end > 0) {
        char c = dst[end];
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

    dst[end + 1] = '\0';
}

void formatPercentageInternal(char *dst, uint32_t value) {
    unsigned int decimalPart = value % DENOMINATOR;
    unsigned int integerPart = (value - decimalPart) / DENOMINATOR;

    SNPRINTF(dst, "%u.%07u", integerPart, decimalPart);
    removeRedundantDecimals(dst);

    size_t totalLength = strlen(dst);
    strcpy(dst + totalLength, " %");
}

void formatTransferRate(char *dst, uint32_t value) {
    if (value == 0) {
        strcpy(dst, "0 %");
    } else if (value < 1000000000) {
        strcpy(dst, "Invalid value");
    } else {
        formatPercentageInternal(dst, value - 1000000000);
    }
}

void formatQuality(char *dst, uint32_t value) {
    if (value == 0) {
        strcpy(dst, "100 %");
    } else {
        formatPercentageInternal(dst, value);
    }
}

void formatPercentage(field_t* field, char *dst) {
    uint32_t value = readUnsigned32(field->data);

    if (field->id == XRP_UINT32_TRANSFER_RATE) {
        formatTransferRate(dst, value);
    } else {
        formatQuality(dst, value);
    }
}