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
        CASE(TRANSACTION_ESCROW_CREATE, "EscrowCreate")
        CASE(TRANSACTION_ESCROW_FINISH, "EscrowFinish")
        CASE(TRANSACTION_ACCOUNT_SET, "AccountSet")
        CASE(TRANSACTION_ESCROW_CANCEL, "EscrowCancel")
        CASE(TRANSACTION_SET_REGULAR_KEY, "SetRegularKey")
        CASE(TRANSACTION_OFFER_CREATE, "OfferCreate")
        CASE(TRANSACTION_OFFER_CANCEL, "OfferCancel")
        CASE(TRANSACTION_SIGNER_LIST_SET, "SignerListSet")
        CASE(TRANSACTION_PAYMENT_CHANNEL_CREATE, "ChannelCreate")
        CASE(TRANSACTION_PAYMENT_CHANNEL_FUND, "ChannelFund")
        CASE(TRANSACTION_PAYMENT_CHANNEL_CLAIM, "ChannelClaim")
        CASE(TRANSACTION_CHECK_CREATE, "CheckCreate")
        CASE(TRANSACTION_CHECK_CASH, "CheckCash")
        CASE(TRANSACTION_CHECK_CANCEL, "CheckCancel")
        CASE(TRANSACTION_DEPOSIT_PREAUTH, "DepositPreauth")
        CASE(TRANSACTION_TRUST_SET, "TrustSet")
        CASE(TRANSACTION_ACCOUNT_DELETE, "AccountDelete")
        default:
            strcpy(dst, "Unknown");
    }
}

bool isTransactionTypeField(field_t *field) {
    return field->dataType == STI_UINT16 && field->id == XRP_UINT16_TRANSACTION_TYPE;
}
