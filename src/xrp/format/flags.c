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

#include "flags.h"
#include "readers.h"
#include "stringUtils.h"
#include "../../apdu/messages/signTransaction.h"
#include "transactionTypes.h"
#include "format.h"
#include <string.h>

#define HAS_FLAG(value, flag) ((value) & (flag)) == flag

bool isFlag(field_t *field) {
    return field->dataType == STI_UINT32 &&
           (field->id == XRP_UINT32_FLAGS || field->id == XRP_UINT32_SET_FLAG ||
            field->id == XRP_UINT32_CLEAR_FLAG);
}

bool isFlagHidden(field_t *field) {
    if (isFlag(field)) {
        uint32_t value = readUnsigned32(field->data);

        return value == 0 || value == TF_FULLY_CANONICAL_SIG;
    }

    return false;
}

void formatAccountSetTransactionFlags(uint32_t value, char *dst) {
// AccountSet flags
#define TF_REQUIRE_DEST_TAG  0x00010000u
#define TF_OPTIONAL_DEST_TAG 0x00020000u
#define TF_REQUIRE_AUTH      0x00040000u
#define TF_OPTIONAL_AUTH     0x00080000u
#define TF_DISALLOW_XRP      0x00100000u
#define TF_ALLOW_XRP         0x00200000u

    if (HAS_FLAG(value, TF_REQUIRE_DEST_TAG)) {
        dst = appendItem(dst, "Require Dest Tag");
    }

    if (HAS_FLAG(value, TF_OPTIONAL_DEST_TAG)) {
        dst = appendItem(dst, "Optional Dest Tag");
    }

    if (HAS_FLAG(value, TF_REQUIRE_AUTH)) {
        dst = appendItem(dst, "Require Auth");
    }

    if (HAS_FLAG(value, TF_OPTIONAL_AUTH)) {
        dst = appendItem(dst, "Optional Auth");
    }

    if (HAS_FLAG(value, TF_DISALLOW_XRP)) {
        dst = appendItem(dst, "Disallow XRP");
    }

    if (HAS_FLAG(value, TF_ALLOW_XRP)) {
        dst = appendItem(dst, "Allow XRP");
    }
}

void formatAccountSetFieldFlags(uint32_t value, char *dst) {
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
            strcpy(dst, "Track Txn ID");
            break;
        case ASF_DEFAULT_RIPPLE:
            strcpy(dst, "Ripple by default");
            break;
        case ASF_DEPOSIT_AUTH:
            strcpy(dst, "Deposit Auth");
            break;
        case ASF_DISABLE_MASTER:
            strcpy(dst, "Disable Master");
            break;
        case ASF_DISALLOW_XRP:
            strcpy(dst, "Disallow XRP");
            break;
        case ASF_GLOBAL_FREEZE:
            strcpy(dst, "Global Freeze");
            break;
        case ASF_NO_FREEZE:
            strcpy(dst, "No Freeze");
            break;
        case ASF_REQUIRE_AUTH:
            strcpy(dst, "Require Auth");
            break;
        case ASF_REQUIRE_DEST:
            strcpy(dst, "Require Dest");
            break;
        default:
            SNPRINTF(dst, "Unknown flag: %u", value);
    }
}

void formatOfferCreateFlags(uint32_t value, char *dst) {
// OfferCreate flags
#define TF_PASSIVE             0x00010000u
#define TF_IMMEDIATE_OR_CANCEL 0x00020000u
#define TF_FILL_OR_KILL        0x00040000u
#define TF_SELL                0x00080000u

    if (HAS_FLAG(value, TF_PASSIVE)) {
        dst = appendItem(dst, "Passive");
    }

    if (HAS_FLAG(value, TF_IMMEDIATE_OR_CANCEL)) {
        dst = appendItem(dst, "Immediate or Cancel");
    }

    if (HAS_FLAG(value, TF_FILL_OR_KILL)) {
        dst = appendItem(dst, "Fill or Kill");
    }

    if (HAS_FLAG(value, TF_SELL)) {
        dst = appendItem(dst, "Sell");
    }
}

void formatPaymentFlags(uint32_t value, char *dst) {
// Payment flags
#define TF_NO_RIPPLE_DIRECT 0x00010000u
#define TF_PARTIAL_PAYMENT  0x00020000u
#define TF_LIMIT_QUALITY    0x00040000u

    if (HAS_FLAG(value, TF_NO_RIPPLE_DIRECT)) {
        dst = appendItem(dst, "No Direct Ripple");
    }

    if (HAS_FLAG(value, TF_PARTIAL_PAYMENT)) {
        dst = appendItem(dst, "Partial Payment");
    }

    if (HAS_FLAG(value, TF_LIMIT_QUALITY)) {
        dst = appendItem(dst, "Limit Quality");
    }
}

void formatTrustSetFlags(uint32_t value, char *dst) {
// TrustSet flags
#define TF_SETF_AUTH       0x00010000u
#define TF_SET_NO_RIPPLE   0x00020000u
#define TF_CLEAR_NO_RIPPLE 0x00040000u
#define TF_SET_FREEZE      0x00100000u
#define TF_CLEAR_FREEZE    0x00200000u

    if (HAS_FLAG(value, TF_SETF_AUTH)) {
        dst = appendItem(dst, "Setf Auth");
    }

    if (HAS_FLAG(value, TF_SET_NO_RIPPLE)) {
        dst = appendItem(dst, "Set No Ripple");
    }

    if (HAS_FLAG(value, TF_CLEAR_NO_RIPPLE)) {
        dst = appendItem(dst, "Clear No Ripple");
    }

    if (HAS_FLAG(value, TF_SET_FREEZE)) {
        dst = appendItem(dst, "Set Freeze");
    }

    if (HAS_FLAG(value, TF_CLEAR_FREEZE)) {
        dst = appendItem(dst, "Clear Freeze");
    }
}

void formatPaymentChannelClaimFlags(uint32_t value, char *dst) {
// PaymentChannelClaim flags
#define TF_RENEW 0x00010000u
#define TF_CLOSE 0x00020000u

    if (HAS_FLAG(value, TF_RENEW)) {
        dst = appendItem(dst, "Renew");
    }

    if (HAS_FLAG(value, TF_CLOSE)) {
        dst = appendItem(dst, "Close");
    }
}

void formatFlags(field_t *field, char *dst) {
    uint32_t value = readUnsigned32(field->data);

    switch (parseContext.transactionType) {
        case TRANSACTION_ACCOUNT_SET:
            if (field->id == XRP_UINT32_FLAGS) {
                formatAccountSetTransactionFlags(value, dst);
            } else {
                formatAccountSetFieldFlags(value, dst);
            }
            break;
        case TRANSACTION_OFFER_CREATE:
            formatOfferCreateFlags(value, dst);
            break;
        case TRANSACTION_PAYMENT:
            formatPaymentFlags(value, dst);
            break;
        case TRANSACTION_TRUST_SET:
            formatTrustSetFlags(value, dst);
            break;
        case TRANSACTION_PAYMENT_CHANNEL_CLAIM:
            formatPaymentChannelClaimFlags(value, dst);
            break;
        default:
            SNPRINTF(dst, "No flags for transaction type %d", parseContext.transactionType);
            return;
    }

    // Check if no flags were found (despite isFlagHidden returning false) and respond appropriately
    if (dst[0] == 0x00) {
        strcpy(dst, "Unsupported value");
    }
}
