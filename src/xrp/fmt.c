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

#include "fmt.h"

#include <stdio.h>
#include <string.h>

#include "amount.h"
#include "general.h"
#include "sign_transaction.h"
#include "transaction_types.h"

static bool is_top_level(const field_t* field) {
    return field->array_info.type == ARRAY_NONE && field->object_depth == 0;
}

// A VaultCreate carries the vault's asset, so its AssetsMaximum is shown in
// that asset, as an amount is. No other transaction carries the asset of its
// Number fields (a VaultSet's or a LoanBrokerSet's is the vault's, a
// LoanSet's the broker's vault's): those are shown in the asset's base
// units, labeled as such, and the review warns first.
static xrp_currency_t* find_number_currency(const field_t* field) {
    if (parse_context.transaction_type != TRANSACTION_VAULT_CREATE ||
        field->id != XRP_NUMBER_ASSETS_MAXIMUM || !is_top_level(field)) {
        return NULL;
    }

    parseResult_t* result = &parse_context.result;
    for (uint8_t i = 0; i < result->num_fields; i++) {
        field_t* asset = &result->fields[i];
        if (asset->data_type == STI_ISSUE && asset->id == XRP_ISSUE_ASSET &&
            is_top_level(asset)) {
            // An Issue starts with its currency, all zeros for XRP.
            return (xrp_currency_t*)asset->data.ptr;
        }
    }

    return NULL;
}

// The label of a Number in an asset the transaction does not name. The
// longest Number in range, 100 characters, still fits in a field value with
// it (MAX_FIELD_LEN is 128 on the smallest target).
#define NUMBER_WITHOUT_ASSET_LABEL " (vault asset, base units)"

static bool is_maximum(const field_t* field) {
    return field->data_type == STI_NUMBER &&
           (field->id == XRP_NUMBER_ASSETS_MAXIMUM ||
            field->id == XRP_NUMBER_DEBT_MAXIMUM);
}

// The default of an omitted field (is_default_field): an integer's value,
// formatted as given, or the meaning of a Number, an account or an amount the
// transaction does not carry.
static void format_default_field(field_t* field, field_value_t* dst) {
    switch (field->data_type) {
        case STI_UINT16:
            uint16_formatter(field, dst);
            break;
        case STI_UINT32:
            uint32_formatter(field, dst);
            break;
        case STI_NUMBER:
            strncpy(dst->buf, is_maximum(field) ? "No limit" : "0",
                    sizeof(dst->buf));
            break;
        case STI_ACCOUNT:
            strncpy(dst->buf, "Loan broker's owner", sizeof(dst->buf));
            break;
        default:
            strncpy(dst->buf, "All", sizeof(dst->buf));
            break;
    }
    size_t len = strlen(dst->buf);
    snprintf(dst->buf + len, sizeof(dst->buf) - len, " (default)");
}

// A VaultClawback of an amount of 0 claws back all the holder has
// (rippled 3.4.1 VaultClawback.cpp:282): "XRP 0" reads "All XRP".
static void format_clawback_all(field_value_t* dst) {
    size_t len = strlen(dst->buf);
    if (strcmp(dst->buf, "0") == 0) {
        strncpy(dst->buf, "All", sizeof(dst->buf));
    } else if (len > 2 && strcmp(dst->buf + len - 2, " 0") == 0) {
        char currency[MAX_FIELD_LEN];
        dst->buf[len - 2] = '\x00';
        strncpy(currency, dst->buf, sizeof(currency));
        currency[sizeof(currency) - 1] = '\x00';
        snprintf(dst->buf, sizeof(dst->buf), "All %s", currency);
    }
}

bool is_number_without_asset(field_t* field) {
    if (field->data_type != STI_NUMBER || !is_top_level(field) ||
        is_default_field(field, parse_context.transaction_type)) {
        return false;
    }
    // Zero, and no limit, are the same in any asset.
    return !is_number_zero(field->data.ptr) &&
           find_number_currency(field) == NULL;
}

bool has_number_without_asset(parseResult_t* transaction) {
    for (uint8_t i = 0; i < transaction->num_fields; i++) {
        if (is_number_without_asset(&transaction->fields[i])) {
            return true;
        }
    }
    return false;
}

void format_field(field_t* field, field_value_t* dst) {
    if (field == NULL || dst == NULL) {
        return;
    }

    memset(dst->buf, '\x00', sizeof(dst->buf));

    if (is_default_field(field, parse_context.transaction_type)) {
        format_default_field(field, dst);
        return;
    }

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
            if (parse_context.transaction_type == TRANSACTION_VAULT_CLAWBACK &&
                field->id == XRP_UINT64_AMOUNT && is_top_level(field)) {
                format_clawback_all(dst);
            }
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
        case STI_NUMBER: {
            xrp_currency_t* currency = find_number_currency(field);
            if (is_maximum(field) && is_top_level(field) &&
                is_number_zero(field->data.ptr)) {
                // rippled 3.4.1: a maximum of 0 is no maximum
                // (VaultSet.cpp:147, LoanSet.cpp:553)
                strncpy(dst->buf, "No limit", sizeof(dst->buf));
            } else if (currency != NULL) {
                asset_number_formatter(field, currency, dst);
            } else {
                number_formatter(field, dst);
                if (is_number_without_asset(field)) {
                    size_t len = strlen(dst->buf);
                    snprintf(dst->buf + len, sizeof(dst->buf) - len,
                             NUMBER_WITHOUT_ASSET_LABEL);
                }
            }
            break;
        }
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
