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

#include "limitations.h"

typedef enum {
    // Normal field types
    STI_UINT16 = 0x01,
    STI_UINT32 = 0x02,
    STI_HASH128 = 0x04,
    STI_HASH256 = 0x05,
    STI_AMOUNT = 0x06,
    STI_VL = 0x07,
    STI_ACCOUNT = 0x08,
    STI_OBJECT = 0x0E,
    STI_ARRAY = 0x0F,
    STI_UINT8 = 0x10,
    STI_PATHSET = 0x12,
    STI_VECTOR256 = 0x13,
    STI_ISSUE = 0x18,

    // Custom field types
    STI_CURRENCY = 0xF0,
} field_type_t;

// Small collection of used field IDs

// UINT8
#define XRP_UINT8_TICK_SIZE 0x10
// UINT16
#define XRP_UINT16_TRANSACTION_TYPE 0x02
#define XRP_UINT16_SIGNER_WEIGHT    0x03
#define XRP_UINT16_TRANSFER_FEE     0x04
#define XRP_UINT16_TRADING_FEE      0x05
#define XRP_UINT16_DISCOUNTED_FEE   0x06
// UINT32
#define XRP_UINT32_NETWORK_ID            0x01
#define XRP_UINT32_FLAGS                 0x02
#define XRP_UINT32_SOURCE_TAG            0x03
#define XRP_UINT32_SEQUENCE              0x04
#define XRP_UINT32_EXPIRATION            0x0A
#define XRP_UINT32_TRANSFER_RATE         0x0B
#define XRP_UINT32_WALLET_SIZE           0x0C
#define XRP_UINT32_OWNER_COUNT           0x0D
#define XRP_UINT32_DESTINATION_TAG       0x0E
#define XRP_UINT32_QUALITY_IN            0x14
#define XRP_UINT32_QUALITY_OUT           0x15
#define XRP_UINT32_OFFER_SEQUENCE        0x19
#define XRP_UINT32_FIRST_LEDGER_SEQUENCE 0x1A
#define XRP_UINT32_LAST_LEDGER_SEQUENCE  0x1B
#define XRP_UINT32_TRANSACTION_INDEX     0x1C
#define XRP_UINT32_OPERATION_LIMIT       0x1D
#define XRP_UINT32_SET_FLAG              0x21
#define XRP_UINT32_CLEAR_FLAG            0x22
#define XRP_UINT32_SIGNER_QUORUM         0x23
#define XRP_UINT32_CANCEL_AFTER          0x24
#define XRP_UINT32_FINISH_AFTER          0x25
#define XRP_UINT32_SETTLE_DELAY          0x27
#define XRP_UINT32_TICKET_COUNT          0x28
#define XRP_UINT32_TICKET_SEQUENCE       0x29
#define XRP_UINT32_NFTOKEN_TAXON         0x2A
// HASH128
#define XRP_HASH128_EMAIL_HASH 0x01
// HASH256
#define XRP_HASH256_PREVIOUS_TXN_ID    0x05
#define XRP_HASH256_WALLET_LOCATOR     0x07
#define XRP_HASH256_ACCOUNT_TXN_ID     0x09
#define XRP_HASH256_NFTOKEN_ID         0x0A
#define XRP_HASH256_AMM_ID             0x0E
#define XRP_HASH256_INVOICE_ID         0x11
#define XRP_HASH256_DIGEST             0x15
#define XRP_HASH256_CHANNEL            0x16
#define XRP_HASH256_CHECK_ID           0x18
#define XRP_HASH256_NFTOKEN_BUY_OFFER  0x1C
#define XRP_HASH256_NFTOKEN_SELL_OFFER 0x1D
// AMOUNT
#define XRP_UINT64_AMOUNT                    0x01
#define XRP_UINT64_BALANCE                   0x02
#define XRP_UINT64_LIMIT_AMOUNT              0x03
#define XRP_UINT64_TAKER_PAYS                0x04
#define XRP_UINT64_TAKER_GETS                0x05
#define XRP_UINT64_FEE                       0x08
#define XRP_UINT64_SEND_MAX                  0x09
#define XRP_UINT64_DELIVER_MIN               0x0A
#define XRP_UINT64_AMOUNT2                   0x0B
#define XRP_UINT64_BID_MIN                   0x0C
#define XRP_UINT64_BID_MAX                   0x0D
#define XRP_UINT64_NFTOKEN_BROKER_FEE        0x13
#define XRP_UINT64_LPTOKEN_OUT               0x19
#define XRP_UINT64_LPTOKEN_IN                0x1A
#define XRP_UINT64_EPRICE                    0x1B
#define XRP_UINT64_PRICE                     0x1C
#define XRP_UINT64_SIGNATURE_REWARD          0x1D
#define XRP_UINT64_MIN_ACCOUNT_CREATE_AMOUNT 0x1E
#define XRP_UINT64_LPTOKEN_BALANCE           0x1F
// VL (Blob)
#define XRP_VL_PUBLIC_KEY      0x01
#define XRP_VL_MESSAGE_KEY     0x02
#define XRP_VL_SIGNING_PUB_KEY 0x03
#define XRP_VL_TXN_SIGNATURE   0x04
#define XRP_VL_URI             0x05
#define XRP_VL_SIGNATURE       0x06
#define XRP_VL_DOMAIN          0x07
#define XRP_VL_MEMO_TYPE       0x0C
#define XRP_VL_MEMO_DATA       0x0D
#define XRP_VL_MEMO_FORMAT     0x0E
#define XRP_VL_FULFILLMENT     0x10
#define XRP_VL_CONDITION       0x11
#define XRP_VL_DID_DOCUMENT    0x1A
#define XRP_VL_DATA            0x1B
// AccountID
#define XRP_ACCOUNT_ACCOUNT                    0x01
#define XRP_ACCOUNT_OWNER                      0x02
#define XRP_ACCOUNT_DESTINATION                0x03
#define XRP_ACCOUNT_ISSUER                     0x04
#define XRP_ACCOUNT_AUTHORIZE                  0x05
#define XRP_ACCOUNT_UNAUTHORIZE                0x06
#define XRP_ACCOUNT_REGULAR_KEY                0x08
#define XRP_ACCOUNT_NFTOKEN_MINTER             0x09
#define XRP_ACCOUNT_OTHER_CHAIN_SOURCE         0x12
#define XRP_ACCOUNT_OTHER_CHAIN_DESTINATION    0x13
#define XRP_ACCOUNT_ATTESTATION_SIGNER_ACCOUNT 0x14
#define XRP_ACCOUNT_ATTESTATION_REWARD_ACCOUNT 0x15
#define XRP_ACCOUNT_LOCKING_CHAIN_DOOR         0x16
#define XRP_ACCOUNT_ISSUING_CHAIN_DOOR         0x17
// VECTOR256
#define XRP_VECTOR256_NF_TOKEN_OFFERS 0x04
// STOBJECT
#define XRP_STOBJECT_MEMO                                                 0x0A
#define XRP_STOBJECT_SIGNER_ENTRY                                         0x0B
#define XRP_STOBJECT_NFTOKEN                                              0x0C
#define XRP_STOBJECT_SIGNER                                               0x10
#define XRP_STOBJECT_AUTH_ACCOUNT                                         0x1B
// STARRAY
#define XRP_STARRAY_SIGNERS                            0x03
#define XRP_STARRAY_SIGNER_ENTRIES                     0x04
#define XRP_STARRAY_MEMOS                              0x09
#define XRP_STARRAY_NFTOKENS                           0x0A
#define XRP_STARRAY_MAJORITIES                         0x10
#define XRP_STARRAY_DISABLED_VALIDATORS                0x11
#define XRP_STARRAY_AUTH_ACCOUNTS                      0x19
// ISSUE
#define XRP_ISSUE_ASSET               0x03
#define XRP_ISSUE_ASSET2              0x04
// CURRENCY
#define XRP_CURRENCY_CURRENCY 0x01

// Array of type one is reserved for end-of-array marker so this
// constant cannot possibly collide with anything in the future
#define ARRAY_PATHSET 0x01
#define ARRAY_NONE    0x00

#define PATHSET_NEXT 0xFF
#define PATHSET_END  0x00

#define XRP_ACCOUNT_SIZE   20
#define XRP_CURRENCY_SIZE  20
#define XRP_VECTOR256_SIZE 32
#define XRP_ISSUE_SIZE     40

typedef struct {
    uint8_t buf[XRP_ACCOUNT_SIZE];
} xrp_account_t;

typedef struct {
    uint8_t buf[XRP_CURRENCY_SIZE];
} xrp_currency_t;

typedef struct {
    uint8_t buf[XRP_ISSUE_SIZE];
} xrp_issue_t;

typedef struct {
    uint8_t type;
    uint8_t index1;
    uint8_t index2;
} array_info_t;

typedef struct {
    uint8_t buf[16];
} hash128_t;

typedef struct {
    uint8_t buf[32];
} hash256_t;

typedef struct {
    uint8_t id;
    field_type_t data_type;
    uint16_t length;
    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        hash128_t *hash128;
        hash256_t *hash256;
        xrp_account_t *account;
        xrp_currency_t *currency;
        uint8_t *ptr;
    } data;
    array_info_t array_info;
} field_t;

typedef struct {
    char buf[MAX_FIELDNAME_LEN];
} field_name_t;

typedef struct {
    char buf[MAX_FIELD_LEN];
} field_value_t;

bool is_normal_account_field(field_t *field);
const char *resolve_field_name(field_t *field);
bool is_field_hidden(field_t *field);

#endif  // LEDGER_APP_XRP_FIELDS_H
