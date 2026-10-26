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

#ifndef LEDGER_APP_XRP_XRPPARSE_H
#define LEDGER_APP_XRP_XRPPARSE_H

#include <stdbool.h>

#include "os.h"
#include "cx.h"
#include "fields.h"
#include "limitations.h"

// Most inner transactions a Batch can carry (rippled kMaxBatchTxCount)
#define BATCH_MAX_INNER_TXS 8

typedef struct {
    uint16_t transaction_type;
    bool has_inner_flag;
    bool has_account;
    bool has_regular_key;
} batchInnerTx_t;

typedef struct {
    uint8_t num_fields;
    field_t fields[MAX_FIELD_COUNT];
} parseResult_t;

typedef struct {
    uint16_t transaction_type;
    bool has_empty_pub_key;
    uint8_t *data;
    uint32_t length;
    uint32_t offset;
    parseResult_t result;
    uint8_t current_array;
    uint8_t array_index1;
    uint8_t array_index2;
    uint8_t object_depth;
    // Batch only: whether its RawTransactions array was read, and what was
    // read of each inner transaction (one RawTransaction object each)
    bool has_raw_transactions;
    uint8_t inner_count;
    batchInnerTx_t inner[BATCH_MAX_INNER_TXS];
} parseContext_t;

int parse_tx(parseContext_t *parse_context);

// 1-based index of the Batch inner transaction a field belongs to, or 0
uint8_t batch_inner_index(const parseContext_t *context, const field_t *field);

// Type of the transaction a field belongs to: its inner transaction's type
// for a field of a Batch inner transaction, the transaction's type otherwise
uint16_t field_transaction_type(const parseContext_t *context, const field_t *field);

#endif  // LEDGER_APP_XRP_XRPPARSE_H
