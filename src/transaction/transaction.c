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
#include "../ui/transaction/reviewMenu.h"
#include "../ui/other/loading.h"
#include "../apdu/global.h"
#include "../xrp/format/transactionTypes.h"
#include "../xrp/format/fields.h"
#include "../xrp/format/amount.h"
#include "../xrp/format/format.h"
#include "../xrp/format/readers.h"
#include "../xrp/xrpHelpers.h"
#include <string.h>

static action_t approvalAction;
static action_t rejectionAction;

void onApprovalMenuResult(unsigned int result) {
    switch (result) {
        case OPTION_SIGN:
            executeAsync(approvalAction, "Signing...");
            break;
        case OPTION_REJECT:
            rejectionAction();
            break;
        default:
            rejectionAction();
    }
}

static bool check_field(const field_t *field,
                        uint8_t dataType,
                        uint8_t id,
                        bool compare_value,
                        uint64_t value) {
    if (field->dataType != dataType || field->id != id) {
        return false;
    }

    if (!compare_value) {
        return true;
    }

    bool ret;
    switch (dataType) {
        case STI_UINT16:
            ret = (readUnsigned16(field->data) == (uint16_t) value);
            break;
        case STI_UINT32:
            ret = (readUnsigned32(field->data) == (uint32_t) value);
            break;
        case STI_AMOUNT:
            ret = (field->length == XRP_AMOUNT_LEN && readUnsigned64(field->data) == value);
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
bool checkSwapConditionsAndSign(parseResult_t *transaction) {
    if (!called_from_swap) {
        PRINTF("Not called from swap!\n");
        return false;
    }

    if (transaction->numFields != 6) {
        PRINTF("Wrong num fields for swap: %d\n", transaction->numFields);
        return false;
    }

    size_t stepIndex = 0;
    field_t *field = &transaction->fields[stepIndex++];
    // "Transaction Type" field
    if (!check_field(field, STI_UINT16, XRP_UINT16_TRANSACTION_TYPE, true, TRANSACTION_PAYMENT)) {
        return false;
    }

    // "Account" field
    field = &transaction->fields[stepIndex++];
    if (!check_field(field, STI_ACCOUNT, XRP_ACCOUNT_ACCOUNT, false, 0)) {
        return false;
    }

    // "Destination Tag" field
    field = &transaction->fields[stepIndex++];
    if (!check_field(field, STI_UINT32, XRP_VL_MEMO_FORMAT, false, 0)) {
        return false;
    }

    SNPRINTF(approvalStrings.swap.tmp, "%u", readUnsigned32(field->data));
    if (strncmp(approvalStrings.swap.tmp,
                approvalStrings.swap.destination_tag,
                sizeof(approvalStrings.swap.destination_tag)) != 0) {
        return false;
    }

    // "Amount" field
    field = &transaction->fields[stepIndex++];
    uint64_t amount = readUnsigned64(approvalStrings.swap.amount);
    if (amount & 0x4000000000000000) {
        return false;
    }
    amount |= 0x4000000000000000;
    if (!check_field(field, STI_AMOUNT, XRP_UINT64_AMOUNT, true, amount)) {
        return false;
    }

    field = &transaction->fields[stepIndex++];
    // "Fee" field
    uint64_t fee = readUnsigned64(approvalStrings.swap.fees);
    if (fee & 0x4000000000000000) {
        return false;
    }
    fee |= 0x4000000000000000;
    if (!check_field(field, STI_AMOUNT, XRP_UINT64_FEE, true, fee)) {
        return false;
    }

    field = &transaction->fields[stepIndex++];
    if (!check_field(field, STI_ACCOUNT, XRP_ACCOUNT_DESTINATION, false, 0)) {
        return false;
    }

    // "Destination" field
    char destination[41];
    size_t addrLength = xrp_public_key_to_encoded_base58(field->data,
                                                         field->length,
                                                         destination,
                                                         sizeof(destination),
                                                         0,
                                                         1);
    if (strncmp(destination, approvalStrings.swap.address, addrLength) != 0) {
        return false;
    }

    PRINTF("Swap parameters verified by current tx\n");

    return true;
}

void reviewTransaction(parseResult_t *transaction, action_t onApprove, action_t onReject) {
    approvalAction = onApprove;
    rejectionAction = onReject;

    if (called_from_swap) {
        if (checkSwapConditionsAndSign(transaction)) {
            approvalAction();
        } else {
            rejectionAction();
        }
        called_from_swap = false;
        os_sched_exit(0);
    } else {
        displayReviewMenu(transaction, onApprovalMenuResult);
    }
}
