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

#include "fmt.h"
#include "amount.h"
#include "general.h"

void format_field(field_t* field, field_value_t* dst) {
    memset(dst->buf, '\x00', sizeof(dst->buf));

    int ret = 0;
    switch (field->data_type) {
        case STI_UINT8:
            uint8_formatter(field, dst);
            break;
        case STI_UINT16:
            uint16_formatter(field, dst);
            break;
        case STI_UINT32:
            uint32_formatter(field, dst);
            break;
        case STI_HASH128:
            hash_formatter128(field, dst);
            break;
        case STI_HASH256:
            hash_formatter256(field, dst);
            break;
        case STI_AMOUNT:
            amount_formatter(field, dst);
            break;
        case STI_VL:
            blob_formatter(field, dst);
            break;
        case STI_VECTOR256:
            vector_formatter256(field, dst);
            break;
        case STI_ACCOUNT:
            account_formatter(field, dst);
            break;
        case STI_CURRENCY:
            currency_formatter(field, dst);
            break;
        case STI_ISSUE:
            currency_formatter(field, dst);
            break;
        default:
            strncpy(dst->buf, "[Not implemented]", sizeof(dst->buf));
            break;
    }

    if (ret != 0) {
        strncpy(dst->buf, "[ERROR DURING FORMATTING]", sizeof(dst->buf));
        return;
    }

    // Replace a zero-length string with a space because of rendering issues
    if (dst->buf[0] == '\x00') {
        dst->buf[0] = ' ';
    }
}
