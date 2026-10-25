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

#include "xrp_parse.h"

#include <string.h>

#include "amount.h"
#include "array.h"
#include "field_sort.h"
#include "fields.h"
#include "flags.h"
#include "globals.h"
#include "readers.h"
#include "transaction_types.h"
#include "xrp_helpers.h"

#define SUCCESS 0
#define CHECK(x)                  \
    do {                          \
        err = x;                  \
        if (err.err != SUCCESS) { \
            return err;           \
        }                         \
    } while (0)

static bool has_data(parseContext_t* context, uint32_t num_bytes) {
    return context->offset + num_bytes - 1 < context->length;
}

static bool has_field(parseContext_t* context, field_type_t data_type,
                      uint8_t id, bool top_level_only) {
    for (uint8_t i = 0; i < context->result.num_fields; ++i) {
        field_t* field = &context->result.fields[i];
        // Same type and id as what we're looking for.
        bool type_and_id_match =
            field->data_type == data_type && field->id == id;
        // If top-level-only is requested, the field must sit outside any
        // array/object; otherwise any nesting level is fine.
        bool level_matches =
            top_level_only ? field->array_info.type == ARRAY_NONE : true;

        if (type_and_id_match && level_matches) {
            return true;
        }
    }

    return false;
}

// Finds the top-level Flags field, if any, and outputs its value. Run only
// after the whole transaction is parsed, since the byte stream can present
// Flags before TransactionType, and the known-bits check below needs the
// transaction type to already be resolved.
//
// Checks both array_info.type and object_depth to rule out any nesting.
// Also flags a second top-level Flags field via *duplicate, instead of
// silently keeping whichever one was seen first.
static bool find_top_level_flags_value(parseContext_t* context, uint32_t* value,
                                       bool* duplicate) {
    bool found = false;
    *duplicate = false;

    for (uint8_t i = 0; i < context->result.num_fields; ++i) {
        field_t* field = &context->result.fields[i];
        if (field->data_type == STI_UINT32 && field->id == XRP_UINT32_FLAGS &&
            field->array_info.type == ARRAY_NONE && field->object_depth == 0) {
            if (found) {
                *duplicate = true;
                return true;
            }
            *value = field->data.u32;
            found = true;
        }
    }

    return found;
}

static bool is_supported_transaction_type(uint16_t t) {
    switch (t) {
        case TRANSACTION_PAYMENT:
        case TRANSACTION_ESCROW_CREATE:
        case TRANSACTION_ESCROW_FINISH:
        case TRANSACTION_ACCOUNT_SET:
        case TRANSACTION_ESCROW_CANCEL:
        case TRANSACTION_SET_REGULAR_KEY:
        case TRANSACTION_OFFER_CREATE:
        case TRANSACTION_OFFER_CANCEL:
        case TRANSACTION_TICKET_CREATE:
        case TRANSACTION_TICKET_CANCEL:
        case TRANSACTION_SIGNER_LIST_SET:
        case TRANSACTION_PAYMENT_CHANNEL_CREATE:
        case TRANSACTION_PAYMENT_CHANNEL_FUND:
        case TRANSACTION_PAYMENT_CHANNEL_CLAIM:
        case TRANSACTION_CHECK_CREATE:
        case TRANSACTION_CHECK_CASH:
        case TRANSACTION_CHECK_CANCEL:
        case TRANSACTION_DEPOSIT_PREAUTH:
        case TRANSACTION_TRUST_SET:
        case TRANSACTION_ACCOUNT_DELETE:
        case TRANSACTION_NFTOKEN_MINT:
        case TRANSACTION_NFTOKEN_BURN:
        case TRANSACTION_NFTOKEN_CREATE_OFFER:
        case TRANSACTION_NFTOKEN_CANCEL_OFFER:
        case TRANSACTION_CLAWBACK:
        case TRANSACTION_AMM_CREATE:
        case TRANSACTION_AMM_DEPOSIT:
        case TRANSACTION_AMM_WITHDRAW:
        case TRANSACTION_AMM_VOTE:
        case TRANSACTION_AMM_BID:
        case TRANSACTION_AMM_DELETE:
            return true;
        default:
            return false;
    }
}

uint8_t* current_position(parseContext_t* context) {
    return context->data + context->offset;
}

typedef struct err_s {
    int err;
} err_t;

err_t advance_position(parseContext_t* context, uint32_t num_bytes) {
    err_t err;

    if (has_data(context, num_bytes)) {
        context->offset += num_bytes;
        err.err = SUCCESS;
    } else {
        err.err = EXCEPTION_OVERFLOW;
    }

    return err;
}

err_t read_next_byte(parseContext_t* context, uint8_t* result) {
    err_t err;

    if (has_data(context, 1)) {
        *result = *current_position(context);
        err = advance_position(context, 1);
    } else {
        err.err = EXCEPTION_OVERFLOW;
    }

    return err;
}

err_t peak_next_byte(parseContext_t* context, uint8_t* result) {
    err_t err;

    if (has_data(context, 1)) {
        *result = *current_position(context);
        err.err = SUCCESS;
    } else {
        err.err = EXCEPTION_OVERFLOW;
    }

    return err;
}

void append_array_info(parseContext_t* context, field_t* field) {
    if (context->current_array != 0) {
        field->array_info.type = context->current_array;
        field->array_info.index1 = context->array_index1;
        field->array_info.index2 = context->array_index2;
    }
}

err_t append_new_field(parseContext_t* context, field_t** field) {
    err_t err;

    if (context->result.num_fields >= MAX_FIELD_COUNT) {
        err.err = NOT_ENOUGH_SPACE;
        return err;
    }

    *field = &context->result.fields[context->result.num_fields++];
    append_array_info(context, *field);
    (*field)->object_depth = context->object_depth;

    err.err = SUCCESS;
    return err;
}

err_t remove_last_field(parseContext_t* context, field_t* last_field) {
    err_t err;

    if (context->result.num_fields == 0) {
        err.err = INVALID_STATE;
        return err;
    }

    // Verify that the last field in the array corresponds to the field that we
    // are supposed to remove.
    if (&context->result.fields[context->result.num_fields - 1] != last_field) {
        err.err = INVALID_STATE;
        return err;
    }

    // Make sure that all data is cleared so that we don't end up with
    // an unexpected state later on.
    memset(last_field, 0, sizeof(field_t));
    context->result.num_fields--;

    err.err = SUCCESS;
    return err;
}

err_t read_fixed_size_field(parseContext_t* context, field_t* field,
                            uint16_t length) {
    field->data.ptr = current_position(context);
    field->length = length;

    return advance_position(context, length);
}

static err_t read_vl_length(parseContext_t* context, uint16_t* data_length) {
    uint8_t value;
    err_t err;

    *data_length = 0;

    CHECK(read_next_byte(context, &value));

    *data_length = value;
    if (*data_length > 192) {
        if (*data_length > 240) {
            // It is impossible to send a transaction large enough to
            // hold a field with a length of 12481 or greater, so we
            // should never reach this point with a valid transaction
            err.err = INVALID_STATE;
            return err;
        }

        uint8_t length_byte2;
        CHECK(read_next_byte(context, &length_byte2));
        *data_length = 193 + ((*data_length - 193) * 256) + length_byte2;
    }

    err.err = SUCCESS;
    return err;
}

err_t read_variable_length_field(parseContext_t* context, field_t* field) {
    uint16_t data_length;
    err_t err;

    CHECK(read_vl_length(context, &data_length));

    return read_fixed_size_field(context, field, data_length);
}

err_t read_amount(parseContext_t* context, field_t* field) {
    uint8_t first_byte;
    err_t err;

    CHECK(peak_next_byte(context, &first_byte));
    if ((first_byte & 0x80u) == 0 && (first_byte & 0x20u) != 0) {
        // An MPT amount: bit 0x20 set, which no valid XRP amount sets.
        CHECK(read_fixed_size_field(context, field, MPT_AMOUNT_LEN));

        // Without the positive bit it is negative, which no transaction may
        // carry, and the review shows only the magnitude.
        if ((first_byte & 0x40u) == 0) {
            err.err = INVALID_STATE;
            return err;
        }

        // No MPT amount exceeds 0x7FFFFFFFFFFFFFFF.
        if ((field->data.ptr[1] & 0x80u) != 0) {
            err.err = INVALID_STATE;
            return err;
        }

        field_t* issuance;
        CHECK(append_new_field(context, &issuance));
        issuance->data_type = STI_UINT192;
        issuance->id = XRP_UINT192_MPTOKEN_ISSUANCE_ID;
        issuance->data.ptr = field->data.ptr + 1 + XRP_AMOUNT_LEN;
        issuance->length = MPT_ISSUANCE_ID_LEN;
    } else if ((first_byte >> 7u) == 0) {
        CHECK(read_fixed_size_field(context, field, XRP_AMOUNT_LEN));
    } else {
        CHECK(read_fixed_size_field(context, field, ISSUED_CURRENCY_LEN));

        // All-zero currency means "this is XRP", but XRP is the ledger's
        // native token and never has an issuer. This 48-byte form always
        // carries one, so an all-zero currency here is a fake XRP amount.
        if (is_all_zeros(field->data.ptr + 8, XRP_CURRENCY_SIZE) ||
            !is_valid_currency(field->data.ptr + 8)) {
            err.err = INVALID_STATE;
            return err;
        }

        if (has_non_standard_currency(field)) {
            field_t* currency;
            CHECK(append_new_field(context, &currency));
            currency->data_type = STI_CURRENCY;
            currency->id = XRP_CURRENCY_CURRENCY;
            currency->data.currency = (xrp_currency_t*)(field->data.ptr + 8);
            currency->length = XRP_CURRENCY_SIZE;
        }

        field_t* issuer;
        CHECK(append_new_field(context, &issuer));
        issuer->data_type = STI_ACCOUNT;
        issuer->id = XRP_ACCOUNT_ISSUER;
        issuer->data.account = (xrp_account_t*)(field->data.ptr + 28);
        issuer->length = XRP_ACCOUNT_SIZE;
    }

    return err;
}

err_t read_vector256_field(parseContext_t* context, field_t* field) {
    field->data_type = STI_VECTOR256;

    err_t err;
    uint16_t data_length;

    CHECK(read_vl_length(context, &data_length));

    if ((data_length % XRP_VECTOR256_SIZE) != 0) {
        err.err = INVALID_STATE;
        return err;
    }

    uint16_t count = data_length / XRP_VECTOR256_SIZE;
    if (count > (MAX_FIELD_COUNT - context->result.num_fields)) {
        err.err = NOT_ENOUGH_SPACE;
        return err;
    }

    CHECK(read_fixed_size_field(context, field, data_length));
    for (size_t i = 0; i < count; i++) {
        field_t* hash256;
        CHECK(append_new_field(context, &hash256));
        hash256->data_type = STI_HASH256;
        // An entry has no field ID of its own: label it after its vector and
        // let the array index tell entries apart. Only NFTokenOffers and
        // CredentialIDs are named in resolve_field_name(); any other vector's
        // own id resolves to "Unknown" and forces blind signing first.
        hash256->id = field->id == XRP_VECTOR256_CREDENTIAL_IDS
                          ? XRP_HASH256_CREDENTIAL_ID
                          : XRP_HASH256_NFTOKEN_OFFER;
        hash256->data.hash256 =
            (hash256_t*)(field->data.ptr + (i * XRP_VECTOR256_SIZE));
        hash256->length = XRP_VECTOR256_SIZE;
        hash256->array_info.type = field->id;
        hash256->array_info.index1 = (uint8_t)(i + 1);
    }

    return err;
}

err_t read_issue(parseContext_t* context, field_t* field) {
    err_t err;

    if (!has_data(context, 20)) {
        err.err = EXCEPTION_OVERFLOW;
        return err;
    }

    if (!is_all_zeros(context->data + context->offset, 20)) {
        CHECK(read_fixed_size_field(context, field, XRP_ISSUE_SIZE));
        field_t* issuer;
        CHECK(append_new_field(context, &issuer));
        issuer->data_type = STI_ACCOUNT;
        issuer->id = XRP_ACCOUNT_ISSUER;
        issuer->data.account = (xrp_account_t*)(field->data.ptr + 20);
        issuer->length = XRP_ACCOUNT_SIZE;
    } else {
        CHECK(read_fixed_size_field(context, field, XRP_CURRENCY_SIZE));
    }

    return err;
}

void handle_array_field(parseContext_t* context, field_t* field) {
    if (field->id != ARR_END) {
        // Begin array
        context->current_array = field->id;
        context->array_index1 = 0;
        context->array_index2 = 0;
    } else {
        // End array
        context->current_array = ARRAY_NONE;
    }
}

err_t handle_object_field(parseContext_t* context, field_t* field) {
    err_t err;

    if (field->id != OBJ_END) {
        // Normal object, only increment first array index
        context->array_index1++;
        context->object_depth++;

        // Explicitly limit the maximum number of array items
        if (context->array_index1 > MAX_ARRAY_LEN) {
            err.err = NOT_SUPPORTED;
            return err;
        }
    } else if (context->object_depth > 0) {
        context->object_depth--;
    }

    err.err = SUCCESS;
    return err;
}

err_t handle_path_step(parseContext_t* context, field_t* field,
                       uint8_t step_type) {
    field_t* new_field;
    err_t err;

    CHECK(read_fixed_size_field(context, field, 20));

    switch (step_type) {
        case 0x01:
            field->data_type = STI_ACCOUNT;
            field->id = XRP_ACCOUNT_ACCOUNT;
            break;
        case 0x20:
            field->data_type = STI_ACCOUNT;
            field->id = XRP_ACCOUNT_ISSUER;
            break;
        case 0x30:
            // Read the issuer separately
            CHECK(append_new_field(context, &new_field));
            CHECK(handle_path_step(context, new_field, 0x20));

            // Intentional fall through to read the currency,
            // which will be positioned before the issuer
            __attribute__((fallthrough));
        case 0x10:
            field->data_type = STI_CURRENCY;
            field->id = XRP_CURRENCY_CURRENCY;
            break;
        default:
            err.err = INVALID_STATE;
            break;
    }

    return err;
}

err_t handle_path_field(parseContext_t* context, field_t* field) {
    // Set default type to STI_PATHSET, which is hidden
    field->data_type = STI_PATHSET;

    uint8_t current_step;
    err_t err;
    CHECK(read_next_byte(context, &current_step));
    switch (current_step) {
        case PATHSET_NEXT:
            // Indicator for next item. Increase index and continue parsing
            context->array_index1++;
            context->array_index2 = 1;

            // Limit the number of paths to the specified maximum
            if (context->array_index1 > MAX_PATH_COUNT) {
                err.err = INVALID_STATE;
                return err;
            }

            break;
        case PATHSET_END:
            // End of path set, stop parsing
            context->current_array = ARRAY_NONE;
            break;
        default:
            // Verify that the step count is within specified bounds before
            // processing the step data
            if (context->array_index2 > MAX_STEP_COUNT) {
                err.err = INVALID_STATE;
                return err;
            }

            // Actual path step found, perform specific parsing
            CHECK(handle_path_step(context, field, current_step));

            // The array index is incremented here because handle_path_step
            // may append more than one field with the same index
            context->array_index2++;
            break;
    }

    return err;
}

void handle_path_set_field(parseContext_t* context) {
    context->current_array = ARRAY_PATHSET;

    // The code for handling path set fields becomes easier if we
    // begin the arrays at one, which is what we want to present
    // to the user
    context->array_index1 = 1;
    context->array_index2 = 1;
}

err_t read_field_value(parseContext_t* context, field_t* field) {
    err_t err;
    err.err = SUCCESS;

    switch (field->data_type) {
        case STI_UINT8:
            CHECK(read_fixed_size_field(context, field, 1));
            field->data.u8 = field->data.ptr[0];
            break;
        case STI_UINT16:
            CHECK(read_fixed_size_field(context, field, 2));
            field->data.u16 = field->data.ptr[1] | (field->data.ptr[0] << 8);
            break;
        case STI_UINT64:
            err = read_fixed_size_field(context, field, 8);
            break;
        case STI_UINT192:
            err = read_fixed_size_field(context, field, 24);
            break;
        case STI_UINT32:
            CHECK(read_fixed_size_field(context, field, 4));
            field->data.u32 = (uint32_t)field->data.ptr[3] |
                              ((uint32_t)field->data.ptr[2] << 8) |
                              ((uint32_t)field->data.ptr[1] << 16) |
                              ((uint32_t)field->data.ptr[0] << 24);
            break;
        case STI_HASH128:
            err = read_fixed_size_field(context, field, sizeof(hash128_t));
            break;
        case STI_HASH256:
            err = read_fixed_size_field(context, field, sizeof(hash256_t));
            break;
        case STI_AMOUNT:
            err = read_amount(context, field);
            break;
        case STI_VL:
            // Intentional fall-through
        case STI_ACCOUNT:
            err = read_variable_length_field(context, field);
            break;
        case STI_VECTOR256:
            err = read_vector256_field(context, field);
            break;
        case STI_ARRAY:
            handle_array_field(context, field);
            break;
        case STI_OBJECT:
            err = handle_object_field(context, field);
            break;
        case STI_PATHSET:
            handle_path_set_field(context);
            break;
        case STI_ISSUE:
            err = read_issue(context, field);
            break;
        default:
            err.err = NOT_SUPPORTED;
            break;
    }

    return err;
}

err_t read_field_header(parseContext_t* context, field_t* field) {
    uint8_t first_byte;
    err_t err;

    CHECK(read_next_byte(context, &first_byte));

    if (first_byte >> 4u == 0) {
        // Type code >= 16
        uint8_t data_type;
        CHECK(read_next_byte(context, &data_type));
        field->data_type = data_type;
        if (first_byte == 0) {
            // Field code >= 16
            CHECK(read_next_byte(context, &field->id));
        } else {
            // Field code < 16
            field->id = first_byte & 0x0fu;
        }
    } else {
        // Type code < 16
        field->data_type = first_byte >> 4u;
        if ((first_byte & 0x0fu) == 0) {
            // Field code >= 16
            CHECK(read_next_byte(context, &field->id));
        } else {
            // Field code < 16
            field->id = first_byte & 0x0fu;
        }
    }

    return err;
}

err_t post_process_field(parseContext_t* context, field_t* field) {
    err_t err;
    err.err = SUCCESS;

    switch (field->data_type) {
        case STI_UINT16:
            // Trust only the first top-level TransactionType field. A
            // later occurrence, at any nesting level, could disagree with
            // it. Force blind signing in that case.
            if (is_transaction_type_field(field)) {
                // Not inside any array/object.
                bool is_top_level = field->array_info.type == ARRAY_NONE &&
                                    context->object_depth == 0;
                // No top-level TransactionType field has been recorded yet.
                bool is_first_occurrence =
                    context->transaction_type == TRANSACTION_INVALID;

                if (is_top_level && is_first_occurrence) {
                    context->transaction_type = field->data.u16;
                } else {
                    err.err = BLIND_SIGN_REQUIRED;
                    return err;
                }
            }
            break;
        case STI_UINT32:
            break;
        case STI_VL:
            // Detect when SigningPubKey is empty (needed for multi-sign).
            // Defence in depth: only a top-level occurrence counts, so one
            // nested inside an array/object cannot spoof this.
            if (field->id == XRP_VL_SIGNING_PUB_KEY && field->length == 0 &&
                field->array_info.type == ARRAY_NONE &&
                context->object_depth == 0) {
                context->has_empty_pub_key = true;
            }
            break;
        case STI_ACCOUNT:
            // Safety check to capture the illegal case where an account
            // field is not 20 bytes long.
            if (field->length != XRP_ACCOUNT_SIZE) {
                err.err = INVALID_STATE;
                return err;
            }
            break;

        default:
            break;
    }

    return err;
}

err_t post_process_transaction(parseContext_t* context) {
    err_t err;
    err.err = SUCCESS;

    // Append "empty" regular key field when clearing it
    if (context->transaction_type == TRANSACTION_SET_REGULAR_KEY &&
        !has_field(context, STI_ACCOUNT, 8, false)) {
        field_t* field;
        CHECK(append_new_field(context, &field));
        field->data_type = STI_ACCOUNT;
        field->id = XRP_ACCOUNT_REGULAR_KEY;
        field->data.ptr = NULL;  // Special value to indicate empty regular key
    }

    return err;
}

err_t read_field(parseContext_t* context, field_t* field) {
    err_t err;

    if (context->current_array == ARRAY_PATHSET) {
        CHECK(handle_path_field(context, field));
    } else {
        CHECK(read_field_header(context, field));
        CHECK(read_field_value(context, field));
    }

    return post_process_field(context, field);
}

err_t parse_tx_internal(parseContext_t* context) {
    err_t err;

    context->transaction_type = TRANSACTION_INVALID;
    context->has_empty_pub_key = false;
    context->object_depth = 0;

    while (context->offset != context->length) {
        if (context->offset > context->length) {
            err.err = EXCEPTION_OVERFLOW;
            return err;
        }

        field_t* field;
        CHECK(append_new_field(context, &field));
        CHECK(read_field(context, field));

        if (is_field_hidden(field)) {
            // This must be done after all data has been read since we
            // always need to keep track of the input stream position.
            CHECK(remove_last_field(context, field));
        }
    }

    CHECK(post_process_transaction(context));

    if (!has_field(context, STI_UINT16, XRP_UINT16_TRANSACTION_TYPE, true) ||
        !has_field(context, STI_ACCOUNT, XRP_ACCOUNT_ACCOUNT, true)) {
        err.err = NOT_SUPPORTED;
        return err;
    }

    sort_fields(&context->result);

    if (context->transaction_type == TRANSACTION_INVALID ||
        !is_supported_transaction_type(context->transaction_type)) {
        err.err = BLIND_SIGN_REQUIRED;
        return err;
    }

    // An unrecognized flag bit must not clear-sign, since it can't be
    // shown to the user. A second top-level Flags field forces blind
    // signing too, same as a duplicate TransactionType above.
    uint32_t flags_value;
    bool flags_duplicate;
    bool has_flags =
        find_top_level_flags_value(context, &flags_value, &flags_duplicate);
    if (flags_duplicate ||
        (has_flags &&
         has_unknown_flags(context->transaction_type, flags_value))) {
        err.err = BLIND_SIGN_REQUIRED;
        return err;
    }

    // Every field that survived is_field_hidden() would be shown to the
    // user during review. A field the app cannot name is a field it
    // cannot explain, even though it is still part of the signed data.
    // Force blind signing rather than show an unlabeled "Unknown" row
    // next to a normal, otherwise-trustworthy-looking review.
    for (uint8_t i = 0; i < context->result.num_fields; ++i) {
        if (!is_field_recognized(&context->result.fields[i])) {
            err.err = BLIND_SIGN_REQUIRED;
            return err;
        }
    }

    err.err = SUCCESS;
    return err;
}

int parse_tx(parseContext_t* context) {
    err_t err = parse_tx_internal(context);
    return err.err;
}
