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
#include "../xrp/xrp_parse.h"
#include "../xrp/xrp_helpers.h"

typedef enum {
    IDLE,
    WAITING_FOR_MORE,
    PENDING_REVIEW,
} signState_e;

typedef struct swapStrings_t {
    char address[41];
    char destination_tag[11];  // uint32_t => 10 numbers max
    uint64_t amount;
    uint64_t fee;
    /* tmp is used when checking swap parameters. As swapStrings_t is present in a union
     * (approvalStrings_t) with a bigger structure (reviewStrings_t), this element is essentially
     * costless.*/
    char tmp[41];
} swapStrings_t;

typedef struct reviewStrings_t {
    field_name_t field_name;
    field_value_t field_value;
} reviewStrings_t;

typedef union {
    reviewStrings_t review;
    swapStrings_t swap;
} approvalStrings_t;

typedef struct publicKeyContext_t {
    cx_ecfp_public_key_t public_key;
    xrp_address_t address;
    uint8_t chain_code[32];
    bool get_chaincode;
} publicKeyContext_t;

typedef struct transactionContext_t {
    cx_curve_t curve;
    uint8_t path_length;
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t raw_tx[MAX_RAW_TX];
    uint32_t raw_tx_length;
} transactionContext_t;

typedef union {
    publicKeyContext_t public_key_context;
    transactionContext_t transaction_context;
} tmpCtx_t;

extern tmpCtx_t tmp_ctx;
extern signState_e sign_state;
extern approvalStrings_t approval_strings;
extern bool called_from_swap;

void reset_transaction_context();

#endif  // LEDGER_APP_XRP_GLOBAL_H
