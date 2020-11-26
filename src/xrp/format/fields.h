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

#ifndef LEDGER_APP_XRP_FIELDS_H
#define LEDGER_APP_XRP_FIELDS_H

#include <stdint.h>
#include <stdbool.h>

// Normal field types
#define STI_UINT16  0x01
#define STI_UINT32  0x02
#define STI_HASH128 0x04
#define STI_HASH256 0x05
#define STI_AMOUNT  0x06
#define STI_VL      0x07
#define STI_ACCOUNT 0x08
#define STI_OBJECT  0x0E
#define STI_ARRAY   0x0F
#define STI_UINT8   0x10
#define STI_PATHSET 0x12

// Custom field types
#define STI_CURRENCY 0xF0

// Small collection of used field IDs
#define XRP_UINT16_TRANSACTION_TYPE     0x02
#define XRP_UINT32_FLAGS                0x02
#define XRP_UINT32_SEQUENCE             0x04
#define XRP_UINT32_EXPIRATION           0x0A
#define XRP_UINT32_TRANSFER_RATE        0x0B
#define XRP_UINT32_QUALITY_IN           0x14
#define XRP_UINT32_QUALITY_OUT          0x15
#define XRP_UINT32_LAST_LEDGER_SEQUENCE 0x1B
#define XRP_UINT32_SET_FLAG             0x21
#define XRP_UINT32_CLEAR_FLAG           0x22
#define XRP_UINT32_CANCEL_AFTER         0x24
#define XRP_UINT32_FINISH_AFTER         0x25
#define XRP_UINT32_SETTLE_DELAY         0x27
#define XRP_VL_SIGNING_PUB_KEY          0x03
#define XRP_VL_DOMAIN                   0x07
#define XRP_VL_MEMO_TYPE                0x0C
#define XRP_VL_MEMO_DATA                0x0D
#define XRP_VL_MEMO_FORMAT              0x0E
#define XRP_ACCOUNT_ACCOUNT             0x01
#define XRP_ACCOUNT_DESTINATION         0x03
#define XRP_ACCOUNT_ISSUER              0x04
#define XRP_ACCOUNT_REGULAR_KEY         0x08
#define XRP_CURRENCY_CURRENCY           0x01
#define XRP_UINT64_AMOUNT               0x01
#define XRP_UINT64_FEE                  0x08

// Array of type one is reserved for end-of-array marker so this
// constant cannot possibly collide with anything in the future
#define ARRAY_PATHSET 0x01
#define ARRAY_NONE    0x00

#define PATHSET_NEXT 0xFF
#define PATHSET_END  0x00

typedef struct {
    uint8_t type;
    uint8_t index1;
    uint8_t index2;
} array_info_t;

typedef struct {
    uint8_t id;
    uint8_t dataType;
    uint16_t length;
    uint8_t *data;
    array_info_t arrayInfo;
} field_t;

bool isNormalAccountField(field_t *field);
void resolveFieldName(field_t *field, char *dst);
bool isFieldHidden(field_t *field);

#endif  // LEDGER_APP_XRP_FIELDS_H
