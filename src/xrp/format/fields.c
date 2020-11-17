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

#define HIDE(t, i) \
    if (field->dataType == (t) && field->id == (i) && field->arrayInfo.type == 0) return true

bool isNormalAccountField(field_t *field) {
    return field->dataType == STI_ACCOUNT && field->id == XRP_ACCOUNT_ACCOUNT &&
           field->arrayInfo.type == 0;
}

void resolveFieldName(field_t *field, char *dst) {
    if (field->dataType == STI_UINT16) {
        switch (field->id) {
            CASE(2, "Transaction Type")
            CASE(3, "Signer Weight")
        }
    }

    if (field->dataType == STI_UINT32) {
        switch (field->id) {
            // 32-bit integers
            CASE(2, "Flags")
            CASE(3, "Source Tag")
            CASE(4, "Sequence")
            CASE(10, "Expiration")
            CASE(11, "Transfer Rate")
            CASE(12, "Wallet Size")
            CASE(14, "Destination Tag")
            CASE(20, "Quality In")
            CASE(21, "Quality Out")
            CASE(25, "Offer Sequence")
            CASE(27, "Last Ledger Sequence")
            CASE(33, "Set Flag")
            CASE(34, "Clear Flag")
            CASE(35, "Signer Quorum")
            CASE(36, "Cancel After")
            CASE(37, "Finish After")
            CASE(39, "Settle Delay")
        }
    }

    if (field->dataType == STI_HASH128) {
        switch (field->id) { CASE(1, "Email Hash") }
    }

    if (field->dataType == STI_HASH256) {
        switch (field->id) {
            // 256-bit
            CASE(5, "Previous Txn ID")
            CASE(7, "Wallet Locator")
            CASE(9, "Account Txn ID")
            CASE(17, "Invoice ID")
            CASE(20, "Ticket ID")
            CASE(22, "Channel")
            CASE(24, "Check ID")
        }
    }

    if (field->dataType == STI_AMOUNT) {
        switch (field->id) {
            // currency amount
            CASE(1, "Amount")
            CASE(2, "Balance")
            CASE(3, "Limit Amount")
            CASE(4, "Taker Pays")
            CASE(5, "Taker Gets")
            CASE(8, "Fee")
            CASE(9, "Send Max")
            CASE(10, "Deliver Min")
        }
    }

    if (field->dataType == STI_VL) {
        switch (field->id) {
            // variable length (common)
            CASE(1, "Public Key")
            CASE(3, "Sig.PubKey")
            CASE(6, "Signature")
            CASE(2, "Message Key")
            CASE(4, "Txn Sig.")
            CASE(7, "Domain")
            CASE(12, "Memo Type")
            CASE(13, "Memo Data")
            CASE(14, "Memo Fmt")
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
            CASE(8, "Regular Key")
        }
    }

    if (field->dataType == STI_OBJECT) {
        switch (field->id) {
            // inner object
            // OBJECT/1 is reserved for end of object
            CASE(10, "Memo")
            CASE(11, "Signer Entry")
            CASE(16, "Signer")
        }
    }

    if (field->dataType == STI_ARRAY) {
        switch (field->id) {
            // array of objects
            // ARRAY/1 is reserved for end of array
            CASE(3, "Signers")
            CASE(4, "Signer Entries")
            CASE(9, "Memos")
        }
    }

    if (field->dataType == STI_UINT8) {
        switch (field->id) {
            // 8-bit integers
            CASE(16, "Tick Size")
        }
    }

    if (field->dataType == STI_PATHSET) {
        switch (field->id) { CASE(1, "Paths") }
    }

    if (field->dataType == STI_CURRENCY) {
        switch (field->id) { CASE(1, "Currency") }
    }

    // Default case
    strcpy(dst, "Unknown");
}

bool isFieldHidden(field_t *field) {
    HIDE(STI_UINT32, XRP_UINT32_SEQUENCE);
    HIDE(STI_UINT32, XRP_UINT32_LAST_LEDGER_SEQUENCE);
    HIDE(STI_VL, XRP_VL_SIGNING_PUB_KEY);

    if (field->dataType == STI_ARRAY || field->dataType == STI_OBJECT ||
        field->dataType == STI_PATHSET) {
        // Field is only used to instruct parsing code how to handle following fields: don't show
        return true;
    }

    if (isFlagHidden(field)) {
        return true;
    }

    return false;
}
