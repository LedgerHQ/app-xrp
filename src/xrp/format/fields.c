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

#include "fields.h"
#include "string.h"
#include "flags.h"
#include "../../common.h"

#define HIDE(t,i) if (field->dataType == (t) && field->id == (i) && field->arrayInfo.type == 0) return true

bool isNormalAccountField(field_t *field) {
    return field->dataType == STI_ACCOUNT &&
            field->id == XRP_ACCOUNT_ACCOUNT &&
            field->arrayInfo.type == 0;
}

void resolveFieldName(field_t *field, char* dst) {
    if (field->dataType == STI_UINT16) {
        switch (field->id) {
            CASE(2, "TransactionType")
            CASE(3, "SignerWeight")
        }
    }

    if (field->dataType == STI_UINT32) {
        switch (field->id) {
            // 32-bit integers
            CASE(2, "Flags")
            CASE(3, "SourceTag")
            CASE(4, "Sequence")
            CASE(10, "Expiration")
            CASE(11, "TransferRate")
            CASE(12, "WalletSize")
            CASE(14, "DestinationTag")
            CASE(20, "QualityIn")
            CASE(21, "QualityOut")
            CASE(25, "OfferSequence")
            CASE(27, "LastLedgerSequence")
            CASE(33, "SetFlag")
            CASE(34, "ClearFlag")
            CASE(35, "SignerQuorum")
            CASE(36, "CancelAfter")
            CASE(37, "FinishAfter")
            CASE(39, "SettleDelay")
        }
    }

    if (field->dataType == STI_HASH128) {
        switch (field->id) {
            CASE(1, "EmailHash")
        }
    }

    if (field->dataType == STI_HASH256) {
        switch (field->id) {
            // 256-bit
            CASE(5, "PreviousTxnID")
            CASE(7, "WalletLocator")
            CASE(9, "AccountTxnID")
            CASE(17, "InvoiceID")
            CASE(20, "TicketID")
            CASE(22, "Channel")
            CASE(24, "CheckID")
        }
    }

    if (field->dataType == STI_AMOUNT) {
        switch (field->id) {
            // currency amount
            CASE(1, "Amount")
            CASE(2, "Balance")
            CASE(3, "LimitAmount")
            CASE(4, "TakerPays")
            CASE(5, "TakerGets")
            CASE(8, "Fee")
            CASE(9, "SendMax")
            CASE(10, "DeliverMin")
        }
    }

    if (field->dataType == STI_VL) {
        switch (field->id) {
            // variable length (common)
            CASE(1, "PublicKey")
            CASE(3, "Sig.PubKey")
            CASE(6, "Signature")
            CASE(2, "MessageKey")
            CASE(4, "TxnSig.")
            CASE(7, "Domain")
            CASE(12, "MemoType")
            CASE(13, "MemoData")
            CASE(14, "MemoFormat")
            CASE(16, "Fulfillment")
            CASE(17, "Condition")
        }
    }

    if (field->dataType == STI_ACCOUNT) {
        switch (field->id) {
            CASE(1, "Account")
            CASE(2, "Owner")
            CASE(3, "Destination")
            CASE(4, "Issuer")
            CASE(5, "Authorize")
            CASE(6, "Unauthorize")
            CASE(8, "RegularKey")
        }
    }

    if (field->dataType == STI_OBJECT) {
        switch (field->id) {
            // inner object
            // OBJECT/1 is reserved for end of object
            CASE(10, "Memo")
            CASE(11, "SignerEntry")
            CASE(16, "Signer")
        }
    }

    if (field->dataType == STI_ARRAY) {
        switch (field->id) {
            // array of objects
            // ARRAY/1 is reserved for end of array
            CASE(3, "Signers")
            CASE(4, "SignerEntries")
            CASE(9, "Memos")
        }
    }

    if (field->dataType == STI_UINT8) {
        switch (field->id) {
            // 8-bit integers
            CASE(16, "TickSize")
        }
    }

    if (field->dataType == STI_PATHSET) {
        switch (field->id) {
            CASE(1, "Paths")
        }
    }

    if (field->dataType == STI_CURRENCY) {
        switch (field->id) {
            CASE(1, "Currency")
        }
    }

    // Default case
    strcpy(dst, "Unknown");
}

bool isFieldHidden(field_t *field) {
    HIDE(STI_UINT32, XRP_UINT32_SEQUENCE);
    HIDE(STI_UINT32, XRP_UINT32_LAST_LEDGER_SEQUENCE);
    HIDE(STI_VL, XRP_VL_SIGNING_PUB_KEY);

    if (field->dataType == STI_ARRAY || field->dataType == STI_OBJECT || field->dataType == STI_PATHSET) {
        // Field is only used to instruct parsing code how to handle following fields: don't show
        return true;
    }

    if (isFlagHidden(field)) {
        return true;
    }

    return false;
}
