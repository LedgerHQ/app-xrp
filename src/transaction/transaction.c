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
uint8_t checkSwapConditionsAndSign(parseResult_t *transaction) {
    if (!called_from_swap) {
        PRINTF("Not called from swap!\n");
        return BOLOS_FALSE;
    }
    if (transaction->numFields != 6) {
        PRINTF("Wrong num fields for swap: %d\n", transaction->numFields);
        return BOLOS_FALSE;
    }
    uint8_t stepIndex = 0;
    field_t *field = &transaction->fields[stepIndex++];
    // "Transaction Type" field
    if (field->dataType == STI_UINT16 && field->id == XRP_UINT16_TRANSACTION_TYPE &&
        readUnsigned16(field->data) == TRANSACTION_PAYMENT) {
        field = &transaction->fields[stepIndex++];
        // "Account" field
        if (field->dataType == STI_ACCOUNT && field->id == XRP_ACCOUNT_ACCOUNT) {
            field = &transaction->fields[stepIndex++];
            // "Destination Tag" field
            if (field->dataType == STI_UINT32 && field->id == XRP_VL_MEMO_FORMAT &&
                readUnsigned32(field->data) != (uint32_t) 0x00000000) {
                SNPRINTF(approvalStrings.swap.tmp, "%u", readUnsigned32(field->data));
                if (strncmp(approvalStrings.swap.tmp,
                            approvalStrings.swap.destination_tag,
                            sizeof(approvalStrings.swap.destination_tag)) == 0) {
                    field = &transaction->fields[stepIndex++];
                    // "Amount" field
                    if (field->dataType == STI_AMOUNT && field->id == XRP_UINT64_AMOUNT &&
                        field->length == XRP_AMOUNT_LEN &&
                        readUnsigned64(field->data) - (uint64_t) 0x4000000000000000 ==
                            readUnsigned64(approvalStrings.swap.amount)) {
                        field = &transaction->fields[stepIndex++];
                        // "Fee" field
                        if (field->dataType == STI_AMOUNT && field->id == XRP_UINT64_FEE &&
                            field->length == XRP_AMOUNT_LEN &&
                            readUnsigned64(field->data) - (uint64_t) 0x4000000000000000 ==
                                readUnsigned64(approvalStrings.swap.fees)) {
                            field = &transaction->fields[stepIndex++];
                            // "Destination" field
                            char destination[41];
                            if (field->dataType == STI_ACCOUNT &&
                                field->id == XRP_ACCOUNT_DESTINATION) {
                                uint16_t addrLength =
                                    xrp_public_key_to_encoded_base58(field->data,
                                                                     field->length,
                                                                     destination,
                                                                     sizeof(destination),
                                                                     0,
                                                                     1);
                                if (strncmp(destination,
                                            approvalStrings.swap.address,
                                            addrLength) == 0) {
                                    PRINTF("Swap parameters verified by current tx\n");
                                    return BOLOS_TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return BOLOS_FALSE;
}

void reviewTransaction(parseResult_t *transaction, action_t onApprove, action_t onReject) {
    approvalAction = onApprove;
    rejectionAction = onReject;

    if (called_from_swap) {
        if (checkSwapConditionsAndSign(transaction) == BOLOS_TRUE) {
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
