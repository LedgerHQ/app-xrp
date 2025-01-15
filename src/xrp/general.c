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

#include "general.h"
#include "readers.h"
#include "fmt.h"
#include "flags.h"
#include "xrp_helpers.h"
#include "time.h"
#include "ascii_strings.h"
#include "limitations.h"
#include "transaction_types.h"
#include "percentage.h"

#define PAGE_W          16
#define ADDR_DST_OFFSET (PAGE_W * 3 + 2)

void uint8_formatter(field_t* field, field_value_t* dst) {
    snprintf(dst->buf, sizeof(dst->buf), "%u", field->data.u8);
}

static const char* resolve_transaction_name(uint16_t value) {
    switch (value) {
        case TRANSACTION_INVALID:
            return "Sign In";
        case TRANSACTION_PAYMENT:
            return "Payment";
        case TRANSACTION_ESCROW_CREATE:
            return "Create Escrow";
        case TRANSACTION_ESCROW_FINISH:
            return "Finish Escrow";
        case TRANSACTION_ACCOUNT_SET:
            return "Account Setting";
        case TRANSACTION_ESCROW_CANCEL:
            return "Cancel Escrow";
        case TRANSACTION_SET_REGULAR_KEY:
            return "Set Regular Key";
        case TRANSACTION_OFFER_CREATE:
            return "Create Offer";
        case TRANSACTION_OFFER_CANCEL:
            return "Cancel Offer";
        case TRANSACTION_TICKET_CREATE:
            return "Ticket Create";
        case TRANSACTION_TICKET_CANCEL:
            return "Ticket Cancel";
        case TRANSACTION_SIGNER_LIST_SET:
            return "Set Signer List";
        case TRANSACTION_PAYMENT_CHANNEL_CREATE:
            return "Create Channel";
        case TRANSACTION_PAYMENT_CHANNEL_FUND:
            return "Fund Channel";
        case TRANSACTION_PAYMENT_CHANNEL_CLAIM:
            return "Channel Claim";
        case TRANSACTION_CHECK_CREATE:
            return "Create Check";
        case TRANSACTION_CHECK_CASH:
            return "Cash Check";
        case TRANSACTION_CHECK_CANCEL:
            return "Cancel Check";
        case TRANSACTION_DEPOSIT_PREAUTH:
            return "Preauth. Deposit";
        case TRANSACTION_TRUST_SET:
            return "Set Trust Line";
        case TRANSACTION_ACCOUNT_DELETE:
            return "Delete Account";
        case TRANSACTION_NFTOKEN_MINT:
            return "NFToken Mint";
        case TRANSACTION_NFTOKEN_BURN:
            return "NFToken Burn";
        case TRANSACTION_NFTOKEN_CREATE_OFFER:
            return "NFToken Create Offer";
        case TRANSACTION_NFTOKEN_CANCEL_OFFER:
            return "NFToken Cancel Offer";
        case TRANSACTION_NFTOKEN_ACCEPT_OFFER:
            return "NFToken Accept Offer";
        case TRANSACTION_CLAWBACK:
            return "Clawback";
        case TRANSACTION_AMM_CREATE:
            return "AMM Create";
        case TRANSACTION_AMM_DEPOSIT:
            return "AMM Deposit";
        case TRANSACTION_AMM_WITHDRAW:
            return "AMM Withdraw";
        case TRANSACTION_AMM_VOTE:
            return "AMM Vote";
        case TRANSACTION_AMM_BID:
            return "AMM Bid";
        case TRANSACTION_AMM_DELETE:
            return "AMM Delete";
        default:
            return "Unknown";
    }
}

void uint16_formatter(field_t* field, field_value_t* dst) {
    uint16_t value = field->data.u16;

    if (is_transaction_type_field(field)) {
        const char* name = resolve_transaction_name(value);
        strncpy(dst->buf, name, sizeof(dst->buf));
    } else {
        snprintf(dst->buf, sizeof(dst->buf), "%u", value);
    }
}

void uint32_formatter(field_t* field, field_value_t* dst) {
    if (is_flag(field)) {
        format_flags(field, dst);
    } else if (is_time(field)) {
        format_time(field, dst);
    } else if (is_time_delta(field)) {
        format_time_delta(field, dst);
    } else if (is_percentage(field)) {
        format_percentage(field, dst);
    } else {
        uint32_t value = field->data.u32;
        snprintf(dst->buf, sizeof(dst->buf), "%u", value);
    }
}

void hash_formatter128(field_t* field, field_value_t* dst) {
    read_hex(dst->buf,
             sizeof(dst->buf),
             field->data.hash128->buf,
             sizeof(field->data.hash128->buf));
}

void hash_formatter256(field_t* field, field_value_t* dst) {
    read_hex(dst->buf,
             sizeof(dst->buf),
             field->data.hash256->buf,
             sizeof(field->data.hash256->buf));
}

static bool should_format_blob_as_string(field_t* field) {
    switch (field->id) {
        case XRP_VL_DOMAIN:
        case XRP_VL_MEMO_TYPE:
        case XRP_VL_MEMO_FORMAT:
            return true;
        case XRP_VL_MEMO_DATA:
        case XRP_VL_URI:
            return is_purely_ascii(field->data.ptr, field->length, false);
        default:
            return false;
    }
}

void blob_formatter(field_t* field, field_value_t* dst) {
    bool too_long = false;
    size_t max_size = sizeof(dst->buf) - 1;

    if (should_format_blob_as_string(field)) {
        memcpy(dst->buf, field->data.ptr, MIN(max_size, field->length));
        if (field->length > max_size) {
            too_long = true;
        }
    } else {
        too_long = !read_hex(dst->buf, max_size, field->data.ptr, field->length);
    }
    dst->buf[max_size] = '\x00';

    if (too_long) {
        strncpy(dst->buf + sizeof(dst->buf) - 4, "...", 4);
    }
}

void vector_formatter256(field_t* field, field_value_t* dst) {
    uint16_t count = field->length / XRP_VECTOR256_SIZE;
    snprintf(dst->buf, sizeof(dst->buf), "%d", count);
}

void account_formatter(field_t* field, field_value_t* dst) {
    if (field->data.ptr == NULL) {
        strncpy(dst->buf, "[empty]", sizeof(dst->buf));
        return;
    }

    // Write full address to dst + ADDR_DST_OFFSET
    xrp_account_t* account = field->data.account;
    xrp_address_t* address = (xrp_address_t*) (dst->buf + ADDR_DST_OFFSET);
    uint16_t addr_length = xrp_public_key_to_encoded_base58(NULL, account, address, 0);

    if (DISPLAY_SEGMENTED_ADDR && addr_length <= PAGE_W * 3) {
        // If the application is configured to split addresses on the target
        // device we segment the address into three parts of roughly the
        // same length and display them each in the middle of their page
        //
        // The segments are positioned so that the longest segment comes first:
        //   long segment, base segment, base segment

        // 1. Calculate how long every segment is
        uint16_t base_segment_len = addr_length / 3;
        uint16_t long_segment_len = addr_length - 2 * base_segment_len;

        // 2. Calculate the left padding required to center each segment
        uint16_t base_padding = (PAGE_W - base_segment_len) / 2;
        uint16_t long_padding = (PAGE_W - long_segment_len) / 2;

        // 3. Fill all three pages with spaces and copy the every segment
        //    to their corresponding position
        uint8_t* p = (uint8_t*) address;
        memset(dst->buf, ' ', PAGE_W * 3);
        memmove(dst->buf + PAGE_W * 0 + long_padding, p, long_segment_len);
        p += long_segment_len;
        memmove(dst->buf + PAGE_W * 1 + base_padding, p, base_segment_len);
        p += base_segment_len;
        memmove(dst->buf + PAGE_W * 2 + base_padding, p, base_segment_len);

        dst->buf[48] = '\x00';
    } else {
        // Application is configured with normal address formatting, make sure
        // that the address is moved to the beginning of our dst
        memmove(dst->buf, address, addr_length);
        dst->buf[addr_length] = '\x00';
    }
}
