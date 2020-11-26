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

#ifndef LEDGER_APP_XRP_GLOBAL_H
#define LEDGER_APP_XRP_GLOBAL_H

#include <stdbool.h>
#include "constants.h"
#include "limitations.h"
#include "../xrp/parse/xrpParse.h"

typedef enum {
    IDLE,
    WAITING_FOR_MORE,
    PENDING_REVIEW,
} signState_e;

typedef struct swapStrings_t {
    char address[41];
    char destination_tag[11];  // uint32_t => 10 numbers max
    uint8_t amount[8];
    uint8_t fees[8];
    /* tmp is used when checking swap parameters. As swapStrings_t is present in a union
     * (approvalStrings_t) with a bigger structure (reviewStrings_t), this element is essentially
     * costless.*/
    char tmp[41];
} swapStrings_t;

typedef struct reviewStrings_t {
    char fieldName[MAX_FIELDNAME_LEN];
    char fieldValue[MAX_FIELD_LEN];
} reviewStrings_t;

typedef union {
    reviewStrings_t review;
    swapStrings_t swap;
} approvalStrings_t;

typedef struct publicKeyContext_t {
    cx_ecfp_public_key_t publicKey;
    char address[41];
    uint8_t chainCode[32];
    bool getChaincode;
} publicKeyContext_t;

typedef struct transactionContext_t {
    cx_curve_t curve;
    uint8_t pathLength;
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint8_t rawTx[MAX_RAW_TX];
    uint32_t rawTxLength;
} transactionContext_t;

typedef union {
    publicKeyContext_t publicKeyContext;
    transactionContext_t transactionContext;
} tmpCtx_t;

extern tmpCtx_t tmpCtx;
extern signState_e signState;
extern approvalStrings_t approvalStrings;
extern bool called_from_swap;

void resetTransactionContext();

#endif  // LEDGER_APP_XRP_GLOBAL_H
