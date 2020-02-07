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

#include <string.h>
#include "transactionTypes.h"
#include "readers.h"
#include "../../common.h"

void resolveTransactionName(field_t *field, char *dst) {
    uint16_t value = readUnsigned16(field->data);

    switch (value) {
        CASE(TRANSACTION_PAYMENT, "Payment")
        CASE(TRANSACTION_ESCROW_CREATE, "Create Escrow")
        CASE(TRANSACTION_ESCROW_FINISH, "Finish Escrow")
        CASE(TRANSACTION_ACCOUNT_SET, "Account Setting")
        CASE(TRANSACTION_ESCROW_CANCEL, "Cancel Escrow")
        CASE(TRANSACTION_SET_REGULAR_KEY, "Set Regular Key")
        CASE(TRANSACTION_OFFER_CREATE, "Create Offer")
        CASE(TRANSACTION_OFFER_CANCEL, "Cancel Offer")
        CASE(TRANSACTION_SIGNER_LIST_SET, "Set Signer List")
        CASE(TRANSACTION_PAYMENT_CHANNEL_CREATE, "Create Channel")
        CASE(TRANSACTION_PAYMENT_CHANNEL_FUND, "Fund Channel")
        CASE(TRANSACTION_PAYMENT_CHANNEL_CLAIM, "Channel Claim")
        CASE(TRANSACTION_CHECK_CREATE, "Create Check")
        CASE(TRANSACTION_CHECK_CASH, "Cash Check")
        CASE(TRANSACTION_CHECK_CANCEL, "Cancel Check")
        CASE(TRANSACTION_DEPOSIT_PREAUTH, "Preauth. Deposit")
        CASE(TRANSACTION_TRUST_SET, "Set Trust Line")
        CASE(TRANSACTION_ACCOUNT_DELETE, "Delete Account")
        default:
            strcpy(dst, "Unknown");
    }
}

bool isTransactionTypeField(field_t *field) {
    return field->dataType == STI_UINT16 && field->id == XRP_UINT16_TRANSACTION_TYPE;
}
