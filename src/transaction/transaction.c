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

#include "transaction.h"
#include "../ui/transaction/review_menu.h"
#include "../ui/other/loading.h"
#include "../apdu/global.h"
#include "../xrp/transaction_types.h"
#include "../xrp/fields.h"
#include "../xrp/amount.h"
#include "../xrp/format.h"
#include "../xrp/readers.h"
#include "../xrp/xrp_helpers.h"
#include "handle_swap_sign_transaction.h"
#include <string.h>

static action_t approval_action;
static action_t rejection_action;

void on_approval_menu_result(unsigned int result) {
    switch (result) {
        case OPTION_SIGN:
            execute_async(approval_action, "Signing...");
            break;
        case OPTION_REJECT:
            rejection_action();
            break;
        default:
            rejection_action();
    }
}

static bool check_field(const field_t *field,
                        field_type_t data_type,
                        uint8_t id,
                        bool compare_value,
                        uint64_t value) {
    if (field->data_type != data_type || field->id != id) {
        return false;
    }

    if (!compare_value) {
        return true;
    }

    bool ret;
    switch (data_type) {
        case STI_UINT16:
            ret = (field->data.u16 == (uint16_t) value);
            break;
        case STI_UINT32:
            ret = (field->data.u32 == (uint32_t) value);
            break;
        case STI_AMOUNT:
            ret = (field->length == XRP_AMOUNT_LEN && read_unsigned64(field->data.ptr) == value);
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

/*
Check that a previously parsed TX has the rigth shape/content for the app to sign it without user
approval.
Exemple of such a swappable TX (as it would be displayed with the approval flow):
{
    "TransactionType" : "Payment",
    "Account" : "ra7Zr8ddy9tB88RaXL8B87YkqhEJG2vkAJ",
    "DestinationTag" : 98765432,
    "Amount" : "21000000",
    "Fee" : "123",
    "Destination" : "rhBuYom8agWA4s7DFoM7AvsDA9XGkVCJz4"
}
 */
bool check_swap_conditions_and_sign(parseResult_t *transaction) {
    if (!called_from_swap) {
        PRINTF("Not called from swap!\n");
        return false;
    }

    if (transaction->num_fields != 6) {
        PRINTF("Wrong num fields for swap: %d\n", transaction->num_fields);
        return false;
    }

    size_t step_index = 0;
    field_t *field = &transaction->fields[step_index++];
    // "Transaction Type" field
    if (!check_field(field, STI_UINT16, XRP_UINT16_TRANSACTION_TYPE, true, TRANSACTION_PAYMENT)) {
        return false;
    }

    // "Account" field
    field = &transaction->fields[step_index++];
    if (!check_field(field, STI_ACCOUNT, XRP_ACCOUNT_ACCOUNT, false, 0)) {
        return false;
    }

    // "Destination Tag" field
    field = &transaction->fields[step_index++];
    if (!check_field(field, STI_UINT32, XRP_VL_MEMO_FORMAT, false, 0)) {
        return false;
    }

    snprintf(approval_strings.swap.tmp, sizeof(approval_strings.swap.tmp), "%u", field->data.u32);
    if (strncmp(approval_strings.swap.tmp,
                approval_strings.swap.destination_tag,
                sizeof(approval_strings.swap.destination_tag)) != 0) {
        return false;
    }

    // "Amount" field
    field = &transaction->fields[step_index++];
    uint64_t amount = approval_strings.swap.amount;
    if (amount & 0x4000000000000000) {
        return false;
    }
    amount |= 0x4000000000000000;
    if (!check_field(field, STI_AMOUNT, XRP_UINT64_AMOUNT, true, amount)) {
        return false;
    }

    field = &transaction->fields[step_index++];
    // "Fee" field
    uint64_t fee = approval_strings.swap.fee;
    if (fee & 0x4000000000000000) {
        return false;
    }
    fee |= 0x4000000000000000;
    if (!check_field(field, STI_AMOUNT, XRP_UINT64_FEE, true, fee)) {
        return false;
    }

    field = &transaction->fields[step_index++];
    if (!check_field(field, STI_ACCOUNT, XRP_ACCOUNT_DESTINATION, false, 0)) {
        return false;
    }

    // "Destination" field
    xrp_address_t destination;
    xrp_account_t *account = (xrp_account_t *) field->data.account;
    size_t addr_length = xrp_public_key_to_encoded_base58(NULL, account, &destination, 0);
    if (strncmp(destination.buf, approval_strings.swap.address, addr_length) != 0) {
        return false;
    }

    PRINTF("Swap parameters verified by current tx\n");

    return true;
}

void review_transaction(parseResult_t *transaction, action_t on_approve, action_t on_reject) {
    approval_action = on_approve;
    rejection_action = on_reject;

    if (called_from_swap) {
        if (check_swap_conditions_and_sign(transaction)) {
            approval_action();
            finalize_exchange_sign_transaction(true);
        } else {
            rejection_action();
            finalize_exchange_sign_transaction(false);
        }
    } else {
        display_review_menu(transaction, on_approval_menu_result);
    }
}
