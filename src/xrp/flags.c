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

#include "flags.h"
#include "readers.h"
#include "sign_transaction.h"
#include "transaction_types.h"
#include "fmt.h"

#define HAS_FLAG(value, flag) ((value) & (flag)) == flag

bool is_flag(const field_t *field) {
    return field->data_type == STI_UINT32 &&
           (field->id == XRP_UINT32_FLAGS || field->id == XRP_UINT32_SET_FLAG ||
            field->id == XRP_UINT32_CLEAR_FLAG);
}

bool is_flag_hidden(const field_t *field) {
    if (is_flag(field)) {
        uint32_t value = field->data.u32;

        return value == 0 || value == TF_FULLY_CANONICAL_SIG;
    }

    return false;
}

static size_t set_error_value(field_value_t *out) {
    strncpy(out->buf, "[ERROR: FAILED TO APPEND ITEM]", sizeof(out->buf));
    return sizeof(out->buf);
}

static size_t append_item(field_value_t *out, size_t offset, const char *in) {
    size_t len = strlen(in);

    if (offset != 0) {
        if (sizeof(out->buf) - offset < 3) {
            return set_error_value(out);
        }
        out->buf[offset + 0] = ',';
        out->buf[offset + 1] = ' ';
        out->buf[offset + 2] = '\x00';
        offset += 2;
    }

    if (len >= sizeof(out->buf) - offset) {
        return set_error_value(out);
    }

    strncpy(out->buf + offset, in, sizeof(out->buf) - offset);

    return offset + len;
}

static void format_account_set_transaction_flags(uint32_t value, field_value_t *dst) {
// AccountSet flags
#define TF_REQUIRE_DEST_TAG  0x00010000u
#define TF_OPTIONAL_DEST_TAG 0x00020000u
#define TF_REQUIRE_AUTH      0x00040000u
#define TF_OPTIONAL_AUTH     0x00080000u
#define TF_DISALLOW_XRP      0x00100000u
#define TF_ALLOW_XRP         0x00200000u

    size_t offset = 0;
    if (HAS_FLAG(value, TF_REQUIRE_DEST_TAG)) {
        offset = append_item(dst, offset, "Require Dest Tag");
    }

    if (HAS_FLAG(value, TF_OPTIONAL_DEST_TAG)) {
        offset = append_item(dst, offset, "Optional Dest Tag");
    }

    if (HAS_FLAG(value, TF_REQUIRE_AUTH)) {
        offset = append_item(dst, offset, "Require Auth");
    }

    if (HAS_FLAG(value, TF_OPTIONAL_AUTH)) {
        offset = append_item(dst, offset, "Optional Auth");
    }

    if (HAS_FLAG(value, TF_DISALLOW_XRP)) {
        offset = append_item(dst, offset, "Disallow XRP");
    }

    if (HAS_FLAG(value, TF_ALLOW_XRP)) {
        append_item(dst, offset, "Allow XRP");
    }
}

static const char *format_account_set_field_flags(uint32_t value) {
// AccountSet flags for fields SetFlag and ClearFlag
#define ASF_ACCOUNT_TXN_ID 5
#define ASF_DEFAULT_RIPPLE 8
#define ASF_DEPOSIT_AUTH   9
#define ASF_DISABLE_MASTER 4
#define ASF_DISALLOW_XRP   3
#define ASF_GLOBAL_FREEZE  7
#define ASF_NO_FREEZE      6
#define ASF_REQUIRE_AUTH   2
#define ASF_REQUIRE_DEST   1

    // Logic is different because only one flag is allowed per field
    switch (value) {
        case ASF_ACCOUNT_TXN_ID:
            return "Track Txn ID";
        case ASF_DEFAULT_RIPPLE:
            return "Ripple by default";
        case ASF_DEPOSIT_AUTH:
            return "Deposit Auth";
        case ASF_DISABLE_MASTER:
            return "Disable Master";
        case ASF_DISALLOW_XRP:
            return "Disallow XRP";
        case ASF_GLOBAL_FREEZE:
            return "Global Freeze";
        case ASF_NO_FREEZE:
            return "No Freeze";
        case ASF_REQUIRE_AUTH:
            return "Require Auth";
        case ASF_REQUIRE_DEST:
            return "Require Dest";
        default:
            return NULL;
    }
}

static void format_account_set_flags(field_t *field, uint32_t value, field_value_t *dst) {
    if (field->id == XRP_UINT32_FLAGS) {
        format_account_set_transaction_flags(value, dst);
    } else {
        const char *flag = format_account_set_field_flags(value);
        if (flag != NULL) {
            strncpy(dst->buf, flag, sizeof(dst->buf));
        } else {
            snprintf(dst->buf, sizeof(dst->buf), "Unknown flag: %u", value);
        }
    }
}

static void format_offer_create_flags(uint32_t value, field_value_t *dst) {
// OfferCreate flags
#define TF_PASSIVE             0x00010000u
#define TF_IMMEDIATE_OR_CANCEL 0x00020000u
#define TF_FILL_OR_KILL        0x00040000u
#define TF_SELL                0x00080000u

    size_t offset = 0;
    if (HAS_FLAG(value, TF_PASSIVE)) {
        offset = append_item(dst, offset, "Passive");
    }

    if (HAS_FLAG(value, TF_IMMEDIATE_OR_CANCEL)) {
        offset = append_item(dst, offset, "Immediate or Cancel");
    }

    if (HAS_FLAG(value, TF_FILL_OR_KILL)) {
        offset = append_item(dst, offset, "Fill or Kill");
    }

    if (HAS_FLAG(value, TF_SELL)) {
        append_item(dst, offset, "Sell");
    }
}

static void format_payment_flags(uint32_t value, field_value_t *dst) {
// Payment flags
#define TF_NO_RIPPLE_DIRECT 0x00010000u
#define TF_PARTIAL_PAYMENT  0x00020000u
#define TF_LIMIT_QUALITY    0x00040000u

    size_t offset = 0;
    if (HAS_FLAG(value, TF_NO_RIPPLE_DIRECT)) {
        offset = append_item(dst, offset, "No Direct Ripple");
    }

    if (HAS_FLAG(value, TF_PARTIAL_PAYMENT)) {
        offset = append_item(dst, offset, "Partial Payment");
    }

    if (HAS_FLAG(value, TF_LIMIT_QUALITY)) {
        append_item(dst, offset, "Limit Quality");
    }
}

static void format_trust_set_flags(uint32_t value, field_value_t *dst) {
// TrustSet flags
#define TF_SETF_AUTH       0x00010000u
#define TF_SET_NO_RIPPLE   0x00020000u
#define TF_CLEAR_NO_RIPPLE 0x00040000u
#define TF_SET_FREEZE      0x00100000u
#define TF_CLEAR_FREEZE    0x00200000u

    size_t offset = 0;
    if (HAS_FLAG(value, TF_SETF_AUTH)) {
        offset = append_item(dst, offset, "Setf Auth");
    }

    if (HAS_FLAG(value, TF_SET_NO_RIPPLE)) {
        offset = append_item(dst, offset, "Set No Ripple");
    }

    if (HAS_FLAG(value, TF_CLEAR_NO_RIPPLE)) {
        offset = append_item(dst, offset, "Clear No Ripple");
    }

    if (HAS_FLAG(value, TF_SET_FREEZE)) {
        offset = append_item(dst, offset, "Set Freeze");
    }

    if (HAS_FLAG(value, TF_CLEAR_FREEZE)) {
        append_item(dst, offset, "Clear Freeze");
    }
}

static void format_payment_channel_claim_flags(uint32_t value, field_value_t *dst) {
// PaymentChannelClaim flags
#define TF_RENEW 0x00010000u
#define TF_CLOSE 0x00020000u

    size_t offset = 0;
    if (HAS_FLAG(value, TF_RENEW)) {
        offset = append_item(dst, offset, "Renew");
    }

    if (HAS_FLAG(value, TF_CLOSE)) {
        append_item(dst, offset, "Close");
    }
}

void format_flags(field_t *field, field_value_t *dst) {
    uint32_t value = field->data.u32;
    switch (parse_context.transaction_type) {
        case TRANSACTION_ACCOUNT_SET:
            format_account_set_flags(field, value, dst);
            break;
        case TRANSACTION_OFFER_CREATE:
            format_offer_create_flags(value, dst);
            break;
        case TRANSACTION_PAYMENT:
            format_payment_flags(value, dst);
            break;
        case TRANSACTION_TRUST_SET:
            format_trust_set_flags(value, dst);
            break;
        case TRANSACTION_PAYMENT_CHANNEL_CLAIM:
            format_payment_channel_claim_flags(value, dst);
            break;
        default:
            snprintf(dst->buf,
                     sizeof(dst->buf),
                     "No flags for transaction type %d",
                     parse_context.transaction_type);
            return;
    }

    // Check if no flags were found (despite is_flag_hidden returning false) and respond
    // appropriately
    if (dst->buf[0] == 0x00) {
        strncpy(dst->buf, "Unsupported value", sizeof(dst->buf));
    }
}
