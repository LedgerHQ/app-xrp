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

#ifndef LEDGER_APP_XRP_TRANSACTIONTYPES_H
#define LEDGER_APP_XRP_TRANSACTIONTYPES_H

#include <stdbool.h>

#include "xrp_parse.h"

#define TRANSACTION_INVALID                0xFFFF
#define TRANSACTION_PAYMENT                0
#define TRANSACTION_ESCROW_CREATE          1
#define TRANSACTION_ESCROW_FINISH          2
#define TRANSACTION_ACCOUNT_SET            3
#define TRANSACTION_ESCROW_CANCEL          4
#define TRANSACTION_SET_REGULAR_KEY        5
#define TRANSACTION_OFFER_CREATE           7
#define TRANSACTION_OFFER_CANCEL           8
#define TRANSACTION_SIGNER_LIST_SET        12
#define TRANSACTION_PAYMENT_CHANNEL_CREATE 13
#define TRANSACTION_PAYMENT_CHANNEL_FUND   14
#define TRANSACTION_PAYMENT_CHANNEL_CLAIM  15
#define TRANSACTION_CHECK_CREATE           16
#define TRANSACTION_CHECK_CASH             17
#define TRANSACTION_CHECK_CANCEL           18
#define TRANSACTION_DEPOSIT_PREAUTH        19
#define TRANSACTION_TRUST_SET              20
#define TRANSACTION_ACCOUNT_DELETE         21

static inline bool is_transaction_type_field(field_t *field) {
    return field->data_type == STI_UINT16 && field->id == XRP_UINT16_TRANSACTION_TYPE;
}

#endif  // LEDGER_APP_XRP_TRANSACTIONTYPES_H
