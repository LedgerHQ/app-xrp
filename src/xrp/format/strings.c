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

#include <os.h>
#include "strings.h"

bool shouldFormatAsString(field_t *field) {
    if (field->dataType != STI_VL) {
        // This function should only be used to determine if fields with
        // type STI_VL should be formatted as a strings instead of hex
        THROW(INVALID_PARAMETER);
    }

    switch (field->id) {
        case XRP_VL_DOMAIN:
        case XRP_VL_MEMO_TYPE:
        case XRP_VL_MEMO_FORMAT:
            return true;
        case XRP_VL_MEMO_DATA:
            return isPurelyAscii(field->data, field->length);
        default:
            return false;
    }
}

bool isPurelyAscii(const uint8_t *data, uint16_t length) {
    for (uint16_t i = 0; i < length; ++i) {
        if (data[i] < 32 || data[i] > 126) {
            return false;
        }
    }

    return true;
}
