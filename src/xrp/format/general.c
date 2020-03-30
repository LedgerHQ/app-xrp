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

#include "general.h"
#include "readers.h"
#include "format.h"
#include "flags.h"
#include "../xrpHelpers.h"
#include "time.h"
#include "strings.h"
#include "../../limitations.h"
#include "transactionTypes.h"
#include "percentage.h"
#include <string.h>

#define PAGE_W 16
#define ADDR_DST_OFFSET (PAGE_W * 3 + 2)

void uint8Formatter(field_t* field, char *dst) {
    uint8_t value = readUnsigned8(field->data);
    SNPRINTF(dst, "%u", value);
}

void uint16Formatter(field_t* field, char *dst) {
    if (isTransactionTypeField(field)) {
        resolveTransactionName(field, dst);
    } else {
        uint16_t value = readUnsigned16(field->data);
        SNPRINTF(dst, "%u", value);
    }
}

void uint32Formatter(field_t* field, char *dst) {
    if (isFlag(field)) {
        formatFlags(field, dst);
    } else if (isTime(field)) {
        formatTime(field, dst);
    } else if (isTimeDelta(field)) {
        formatTimeDelta(field, dst);
    } else if (isPercentage(field)) {
        formatPercentage(field, dst);
    } else {
        uint32_t value = readUnsigned32(field->data);
        SNPRINTF(dst, "%u", value);
    }
}

void hashFormatter(field_t* field, char *dst) {
    readHex(dst, field->data, field->length);
}

void blobFormatter(field_t* field, char *dst) {
    bool tooLong = false;
    if (shouldFormatAsString(field)) {
        memcpy(dst, field->data, MIN(MAX_FIELD_LEN - 1, field->length));
        tooLong = (field->length > MAX_FIELD_LEN - 1);
    } else {
        uint16_t bytesToCopy = field->length;
        if (bytesToCopy >= MAX_FIELD_LEN / 2) {
            // Preserve space for the null terminator
            bytesToCopy = MAX_FIELD_LEN / 2 - 1;
            tooLong = true;
        }

        readHex(dst, field->data, bytesToCopy);
    }

    if (tooLong) {
        strcpy(dst + MAX_FIELD_LEN - 4, "...");
    }
}

void accountFormatter(field_t* field, char *dst) {
    if (field->data != NULL) {
        // Write full address to dst + ADDR_DST_OFFSET
        uint16_t addrLength = xrp_public_key_to_encoded_base58(
            field->data, field->length,
            dst + ADDR_DST_OFFSET, MAX_FIELD_LEN - ADDR_DST_OFFSET,
            0, 1
        );

        if (DISPLAY_SEGMENTED_ADDR && addrLength <= PAGE_W * 3) {
            // If the application is configured to split addresses on the target
            // device we segment the address into three parts of roughly the
            // same length and display them each in the middle of their page
            //
            // The segments are positioned so that the longest segment comes first:
            //   long segment, base segment, base segment

            // 1. Calculate how long every segment is
            uint16_t baseSegmentLen = addrLength / 3;
            uint16_t longSegmentLen = addrLength - 2 * baseSegmentLen;

            // 2. Calculate the left padding required to center each segment
            uint16_t basePadding = (PAGE_W - baseSegmentLen) / 2;
            uint16_t longPadding = (PAGE_W - longSegmentLen) / 2;

            // 3. Fill all three pages with spaces and copy the every segment
            //    to their corresponding position
            os_memset(dst, ' ', PAGE_W * 3);
            os_memmove(dst + PAGE_W * 0 + longPadding, dst + ADDR_DST_OFFSET, longSegmentLen);
            os_memmove(dst + PAGE_W * 1 + basePadding, dst + ADDR_DST_OFFSET + longSegmentLen, baseSegmentLen);
            os_memmove(dst + PAGE_W * 2 + basePadding, dst + ADDR_DST_OFFSET + longSegmentLen + baseSegmentLen, baseSegmentLen);

            dst[48] = 0;
        } else {
            // Application is configured with normal address formatting, make sure
            // that the address is moved to the beginning of our dst
            os_memmove(dst, dst + 50, addrLength);
            dst[addrLength] = 0;
        }
    } else {
        strcpy(dst, "[empty]");
    }
}
