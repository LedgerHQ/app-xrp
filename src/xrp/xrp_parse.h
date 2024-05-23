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
} parseContext_t;

int parse_tx(parseContext_t *parse_context);

#endif  // LEDGER_APP_XRP_XRPPARSE_H
