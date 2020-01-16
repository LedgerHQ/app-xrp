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
#include "format.h"
#include "amount.h"
#include "general.h"
#include "fields.h"
#include "../xrpHelpers.h"
#include "../../limitations.h"

typedef void (*fieldFormatter_t)(field_t* field, char* dst);

fieldFormatter_t getFormatter(field_t* field) {
    switch (field->dataType) {
        case STI_UINT8:
            return uint8Formatter;
        case STI_UINT16:
            return uint16Formatter;
        case STI_UINT32:
            return uint32Formatter;
        case STI_HASH128:
            // Intentional fall-through
        case STI_HASH256:
            return hashFormatter;
        case STI_AMOUNT:
            return amountFormatter;
        case STI_VL:
            return blobFormatter;
        case STI_ACCOUNT:
            return accountFormatter;
        case STI_CURRENCY:
            return currencyFormatter;
        default:
            return NULL;
    }
}

void formatField(field_t* field, char* dst) {
    memset(dst, 0, MAX_FIELD_LEN);

    fieldFormatter_t formatter = getFormatter(field);
    if (formatter != NULL) {
        formatter(field, dst);
    } else {
        strcpy(dst, "[Not implemented]");
    }

    // Replace a zero-length string with a space because of rendering issues
    if (dst[0] == 0x00) {
        dst[0] = ' ';
    }
}
