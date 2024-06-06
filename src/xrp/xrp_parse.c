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

#include <string.h>

#include "xrp_parse.h"
#include "xrp_helpers.h"
#include "amount.h"
#include "array.h"
#include "fields.h"
#include "readers.h"
#include "transaction_types.h"
#include "field_sort.h"
#include "flags.h"

#define SUCCESS 0
#define CHECK(x)                  \
    do {                          \
        err = x;                  \
        if (err.err != SUCCESS) { \
            return err;           \
        }                         \
    } while (0)

static bool has_data(parseContext_t *context, uint32_t num_bytes) {
    return context->offset + num_bytes - 1 < context->length;
}

static bool has_field(parseContext_t *context, field_type_t data_type, uint8_t id) {
    for (uint8_t i = 0; i < context->result.num_fields; ++i) {
        field_t *field = &context->result.fields[i];
        if (field->data_type == data_type && field->id == id) {
            return true;
        }
    }

    return false;
}

uint8_t *current_position(parseContext_t *context) {
    return context->data + context->offset;
}

typedef struct err_s {
    int err;
} err_t;

err_t advance_position(parseContext_t *context, uint32_t num_bytes) {
    err_t err;

    if (has_data(context, num_bytes)) {
        context->offset += num_bytes;
        err.err = SUCCESS;
    } else {
        err.err = EXCEPTION_OVERFLOW;
    }

    return err;
}

err_t read_next_byte(parseContext_t *context, uint8_t *result) {
    err_t err;

    if (has_data(context, 1)) {
        *result = *current_position(context);
        err = advance_position(context, 1);
    } else {
        err.err = EXCEPTION_OVERFLOW;
    }

    return err;
}

err_t peak_next_byte(parseContext_t *context, uint8_t *result) {
    err_t err;

    if (has_data(context, 1)) {
        *result = *current_position(context);
        err.err = SUCCESS;
    } else {
        err.err = EXCEPTION_OVERFLOW;
    }

    return err;
}

void append_array_info(parseContext_t *context, field_t *field) {
    if (context->current_array != 0) {
        field->array_info.type = context->current_array;
        field->array_info.index1 = context->array_index1;
        field->array_info.index2 = context->array_index2;
    }
}

err_t append_new_field(parseContext_t *context, field_t **field) {
    err_t err;

    if (context->result.num_fields >= MAX_FIELD_COUNT) {
        err.err = NOT_ENOUGH_SPACE;
        return err;
    }

    *field = &context->result.fields[context->result.num_fields++];
    append_array_info(context, *field);

    err.err = SUCCESS;
    return err;
}

err_t remove_last_field(parseContext_t *context, field_t *last_field) {
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

err_t read_fixed_size_field(parseContext_t *context, field_t *field, uint16_t length) {
    field->data.ptr = current_position(context);
    field->length = length;

    return advance_position(context, length);
}

err_t read_variable_length_field(parseContext_t *context, field_t *field) {
    uint8_t value;
    err_t err;

    CHECK(read_next_byte(context, &value));

    uint16_t data_length = value;
    if (data_length > 192) {
        if (data_length > 240) {
            // It is impossible to send a transaction large enough to
            // hold a field with a length of 12481 or greater, so we
            // should never reach this point with a valid transaction
            err.err = INVALID_STATE;
            return err;
        }

        uint8_t length_byte2;
        CHECK(read_next_byte(context, &length_byte2));
        data_length = 193 + ((data_length - 193) * 256) + length_byte2;
    }

    return read_fixed_size_field(context, field, data_length);
}

err_t read_amount(parseContext_t *context, field_t *field) {
    uint8_t first_byte;
    err_t err;

    CHECK(peak_next_byte(context, &first_byte));
    if ((first_byte >> 7u) == 0) {
        CHECK(read_fixed_size_field(context, field, XRP_AMOUNT_LEN));
    } else {
        CHECK(read_fixed_size_field(context, field, ISSUED_CURRENCY_LEN));

        if (has_non_standard_currency(field)) {
            field_t *currency;
            CHECK(append_new_field(context, &currency));
            currency->data_type = STI_CURRENCY;
            currency->id = XRP_CURRENCY_CURRENCY;
            currency->data.currency = (xrp_currency_t *) (field->data.ptr + 8);
            currency->length = XRP_CURRENCY_SIZE;
        }

        field_t *issuer;
        CHECK(append_new_field(context, &issuer));
        issuer->data_type = STI_ACCOUNT;
        issuer->id = XRP_ACCOUNT_ISSUER;
        issuer->data.account = (xrp_account_t *) (field->data.ptr + 28);
        issuer->length = XRP_ACCOUNT_SIZE;
    }

    return err;
}

err_t read_vector256_field(parseContext_t *context, field_t *field) {
    field->data_type = STI_VECTOR256;

    err_t err;
    uint8_t value;

    CHECK(read_next_byte(context, &value));

    uint16_t count = value / XRP_VECTOR256_SIZE;
    read_fixed_size_field(context, field, XRP_VECTOR256_SIZE * count);
    for (size_t i = 0; i < count; i++) {
        field_t *hash256;
        CHECK(append_new_field(context, &hash256));
        hash256->data_type = STI_HASH256;
        hash256->id = XRP_HASH256_NFTOKEN_BUY_OFFER;
        hash256->data.hash256 = (hash256_t *) (field->data.ptr + (i * 32));
        hash256->length = XRP_VECTOR256_SIZE;
    }

    return err;
}

err_t read_issue(parseContext_t *context, field_t *field) {
    err_t err;

    if (!is_all_zeros(context->data + context->offset, 20)) {
        CHECK(read_fixed_size_field(context, field, XRP_ISSUE_SIZE));
        field_t *issuer;
        CHECK(append_new_field(context, &issuer));
        issuer->data_type = STI_ACCOUNT;
        issuer->id = XRP_ACCOUNT_ISSUER;
        issuer->data.account = (xrp_account_t *) (field->data.ptr + 20);
        issuer->length = XRP_ACCOUNT_SIZE;
    } else {
        CHECK(read_fixed_size_field(context, field, XRP_CURRENCY_SIZE));
    }
    
    return err;
}

void handle_array_field(parseContext_t *context, field_t *field) {
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

err_t handle_object_field(parseContext_t *context, field_t *field) {
    err_t err;

    if (field->id != OBJ_END) {
        // Normal object, only increment first array index
        context->array_index1++;

        // Explicitly limit the maximum number of array items
        if (context->array_index1 > MAX_ARRAY_LEN) {
            err.err = NOT_SUPPORTED;
            return err;
        }
    }

    err.err = SUCCESS;
    return err;
}

err_t handle_path_step(parseContext_t *context, field_t *field, uint8_t step_type) {
    field_t *new_field;
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
            field->data_type = STI_PATHSET;
            break;
    }

    return err;
}

err_t handle_path_field(parseContext_t *context, field_t *field) {
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

void handle_path_set_field(parseContext_t *context) {
    context->current_array = ARRAY_PATHSET;

    // The code for handling path set fields becomes easier if we
    // begin the arrays at one, which is what we want to present
    // to the user
    context->array_index1 = 1;
    context->array_index2 = 1;
}

err_t read_field_value(parseContext_t *context, field_t *field) {
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
        case STI_UINT32:
            CHECK(read_fixed_size_field(context, field, 4));
            field->data.u32 = field->data.ptr[3] | (field->data.ptr[2] << 8) |
                              (field->data.ptr[1] << 16) | (field->data.ptr[0] << 24);
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

err_t read_field_header(parseContext_t *context, field_t *field) {
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

err_t post_process_field(parseContext_t *context, field_t *field) {
    err_t err;
    err.err = SUCCESS;

    switch (field->data_type) {
        case STI_UINT16:
            // Record the transaction type since it must be available for the
            // formatting of certain values
            if (is_transaction_type_field(field)) {
                context->transaction_type = field->data.u16;
            }
            break;
        case STI_UINT32:
            break;
        case STI_VL:
            // Detect when SigningPubKey is empty (needed for multi-sign)
            if (field->id == XRP_VL_SIGNING_PUB_KEY && field->length == 0) {
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

err_t post_process_transaction(parseContext_t *context) {
    err_t err;
    err.err = SUCCESS;

    // Append "empty" regular key field when clearing it
    if (context->transaction_type == TRANSACTION_SET_REGULAR_KEY &&
        !has_field(context, STI_ACCOUNT, 8)) {
        field_t *field;
        CHECK(append_new_field(context, &field));
        field->data_type = STI_ACCOUNT;
        field->id = XRP_ACCOUNT_REGULAR_KEY;
        field->data.ptr = NULL;  // Special value to indicate empty regular key
    }

    return err;
}

err_t read_field(parseContext_t *context, field_t *field) {
    err_t err;

    if (context->current_array == ARRAY_PATHSET) {
        CHECK(handle_path_field(context, field));
    } else {
        CHECK(read_field_header(context, field));
        CHECK(read_field_value(context, field));
    }

    return post_process_field(context, field);
}

err_t parse_tx_internal(parseContext_t *context) {
    err_t err;

    context->transaction_type = TRANSACTION_INVALID;
    context->has_empty_pub_key = false;

    while (context->offset != context->length) {
        if (context->offset > context->length) {
            err.err = EXCEPTION_OVERFLOW;
            return err;
        }

        field_t *field;
        CHECK(append_new_field(context, &field));
        CHECK(read_field(context, field));

        if (is_field_hidden(field)) {
            // This must be done after all data has been read since we
            // always need to keep track of the input stream position.
            CHECK(remove_last_field(context, field));
        }
    }

    CHECK(post_process_transaction(context));
    sort_fields(&context->result);

    err.err = SUCCESS;
    return err;
}

int parse_tx(parseContext_t *context) {
    err_t err = parse_tx_internal(context);
    return err.err;
}
