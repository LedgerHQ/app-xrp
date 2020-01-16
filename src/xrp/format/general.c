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
#include <string.h>

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
        xrp_public_key_to_encoded_base58(field->data, field->length, dst, MAX_FIELD_LEN, 0, 1);

        uint16_t addrLength = strlen(dst);
        if (!FULL_ADDR_FORMAT && addrLength > 16) {
            os_memmove(dst + 7, "..", 2);
            os_memmove(dst + 9, dst + addrLength - 7, 7);
            dst[16] = 0;
        }
    } else {
        strcpy(dst, "[empty]");
    }
}
