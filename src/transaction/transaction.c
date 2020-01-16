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
#include "../ui/transaction/approvalMenu.h"
#include "../ui/other/loading.h"

void requestReview();

static parseResult_t *currentTransaction;
static action_t approvalAction;
static action_t rejectionAction;

void onApprovalMenuResult(unsigned int result) {
    switch (result) {
        case OPTION_SIGN:
            executeAsync(approvalAction, "Signing...");
            break;
        case OPTION_REVIEW:
            requestReview();
            break;
        case OPTION_REJECT:
            rejectionAction();
            break;
        default:
            rejectionAction();
    }
}

void requestApproval() {
    displayApprovalMenu(onApprovalMenuResult);
}

void requestReview() {
    displayReviewMenu(currentTransaction, requestApproval);
}

void reviewTransaction(parseResult_t *transaction, action_t onApprove, action_t onReject) {
    currentTransaction = transaction;
    approvalAction = onApprove;
    rejectionAction = onReject;

    requestReview();
}
