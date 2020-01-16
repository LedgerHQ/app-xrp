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
            CASE(1, "LedgerEntryType")
            CASE(2, "TransactionType")
            CASE(3, "SignerWeight")
        }
    }

    if (field->dataType == STI_UINT32) {
        switch (field->id) {
            // 32-bit integers (common)
            CASE(2, "Flags")
            CASE(3, "SourceTag")
            CASE(4, "Sequence")
            CASE(5, "PreviousTxnLgrSeq")
            CASE(6, "LedgerSequence")
            CASE(7, "CloseTime")
            CASE(8, "ParentCloseTime")
            CASE(9, "SigningTime")
            CASE(10, "Expiration")
            CASE(11, "TransferRate")
            CASE(12, "WalletSize")
            CASE(13, "OwnerCount")
            CASE(14, "DestinationTag")

            // 32-bit integers (uncommon)
            CASE(16, "HighQualityIn")
            CASE(17, "HighQualityOut")
            CASE(18, "LowQualityIn")
            CASE(19, "LowQualityOut")
            CASE(20, "QualityIn")
            CASE(21, "QualityOut")
            CASE(22, "StampEscrow")
            CASE(23, "BondAmount")
            CASE(24, "LoadFee")
            CASE(25, "OfferSequence")
            CASE(26, "FirstLedgerSequence")
            CASE(27, "LastLedgerSequence")
            CASE(28, "TransactionIndex")
            CASE(29, "OperationLimit")
            CASE(30, "ReferenceFeeUnits")
            CASE(31, "ReserveBase")
            CASE(32, "ReserveIncrement")
            CASE(33, "SetFlag")
            CASE(34, "ClearFlag")
            CASE(35, "SignerQuorum")
            CASE(36, "CancelAfter")
            CASE(37, "FinishAfter")
            CASE(38, "SignerListID")
            CASE(39, "SettleDelay")
        }
    }

    if (field->dataType == STI_UINT64) {
        switch (field->id) {
            CASE(1, "IndexNext")
            CASE(2, "IndexPrevious")
            CASE(3, "BookNode")
            CASE(4, "OwnerNode")
            CASE(5, "BaseFee")
            CASE(6, "ExchangeRate")
            CASE(7, "LowNode")
            CASE(8, "HighNode")
            CASE(9, "DestinationNode")
            CASE(10,"Cookie")
        }
    }

    if (field->dataType == STI_HASH128) {
        switch (field->id) {
            CASE(1, "EmailHash")
        }
    }

    if (field->dataType == STI_HASH160) {
        switch (field->id) {
            CASE(1, "TakerPaysCurrency")
            CASE(2, "TakerPaysIssuer")
            CASE(3, "TakerGetsCurrency")
            CASE(4, "TakerGetsIssuer")
        }
    }

    if (field->dataType == STI_HASH256) {
        switch (field->id) {
            // 256-bit (common)
            CASE(1, "LedgerHash")
            CASE(2, "ParentHash")
            CASE(3, "TransactionHash")
            CASE(4, "AccountHash")
            CASE(5, "PreviousTxnID")
            CASE(6, "LedgerIndex")
            CASE(7, "WalletLocator")
            CASE(8, "RootIndex")
            CASE(9, "AccountTxnID")

            // 256-bit (uncommon)
            CASE(16, "BookDirectory")
            CASE(17, "InvoiceID")
            CASE(18, "Nickname")
            CASE(19, "Amendment")
            CASE(20, "TicketID")
            CASE(21, "Digest")
            CASE(22, "Channel")
            CASE(23, "ConsensusHash")
            CASE(24, "CheckID")
        }
    }

    if (field->dataType == STI_AMOUNT) {
        switch (field->id) {
            // currency amount (common)
            CASE(1, "Amount")
            CASE(2, "Balance")
            CASE(3, "LimitAmount")
            CASE(4, "TakerPays")
            CASE(5, "TakerGets")
            CASE(6, "LowLimit")
            CASE(7, "HighLimit")
            CASE(8, "Fee")
            CASE(9, "SendMax")
            CASE(10, "DeliverMin")

            // currency amount (uncommon)
            CASE(16, "MinimumOffer")
            CASE(17, "RippleEscrow")
            CASE(18, "DeliveredAmount")
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
            CASE(8, "FundCode")
            CASE(9, "RemoveCode")
            CASE(10, "ExpireCode")
            CASE(11, "CreateCode")
            CASE(12, "MemoType")
            CASE(13, "MemoData")
            CASE(14, "MemoFormat")


            // variable length (uncommon)
            CASE(16, "Fulfillment")
            CASE(17, "Condition")
            CASE(18, "MasterSignature")
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
            CASE(7, "Target")
            CASE(8, "RegularKey")
        }
    }

    if (field->dataType == STI_OBJECT) {
        switch (field->id) {
            // inner object
            // OBJECT/1 is reserved for end of object
            CASE(2, "TransactionMetaData")
            CASE(3, "CreatedNode")
            CASE(4, "DeletedNode")
            CASE(5, "ModifiedNode")
            CASE(6, "PreviousFields")
            CASE(7, "FinalFields")
            CASE(8, "NewFields")
            CASE(9, "TemplateEntry")
            CASE(10, "Memo")
            CASE(11, "SignerEntry")

            // inner object (uncommon)
            CASE(16, "Signer")
            //   17 has not been used yet...
            CASE(18, "Majority")
        }
    }

    if (field->dataType == STI_ARRAY) {
        switch (field->id) {
            // array of objects
            // ARRAY/1 is reserved for end of array
            // CASE(2, "SigningAccounts") // Never been used.
            CASE(3, "Signers")
            CASE(4, "SignerEntries")
            CASE(5, "Template")
            CASE(6, "Necessary")
            CASE(7, "Sufficient")
            CASE(8, "AffectedNodes")
            CASE(9, "Memos")

            // array of objects (uncommon)
            CASE(16, "Majorities")
        }
    }

    if (field->dataType == STI_UINT8) {
        switch (field->id) {
            // 8-bit integers
            CASE(1, "CloseResolution")
            CASE(2, "Method")
            CASE(3, "TransactionResult")

            // 8-bit integers (uncommon)
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
