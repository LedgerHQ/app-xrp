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

#include <string.h>

#include "common.h"
#include "flags.h"
#include "sign_transaction.h"
#include "transaction_types.h"

#define HIDE(t, i)                                     \
    if (field->data_type == (t) && field->id == (i) && \
        field->array_info.type == 0)                   \
    return true

// The defaults, from rippled 3.4.1
const default_field_t DEFAULT_FIELDS[] = {
    // VaultCreate.cpp:260: no limit
    {TRANSACTION_VAULT_CREATE, STI_NUMBER, XRP_NUMBER_ASSETS_MAXIMUM, 0},
    // VaultClawback.cpp:63-76 and 282: all the holder has
    {TRANSACTION_VAULT_CLAWBACK, STI_AMOUNT, XRP_UINT64_AMOUNT, 0},
    // LoanSet.h:65-73: one payment, due every 60 s, with 60 s of grace
    {TRANSACTION_LOAN_SET, STI_UINT32, XRP_UINT32_PAYMENT_TOTAL, 1},
    {TRANSACTION_LOAN_SET, STI_UINT32, XRP_UINT32_PAYMENT_INTERVAL, 60},
    {TRANSACTION_LOAN_SET, STI_UINT32, XRP_UINT32_GRACE_PERIOD, 60},
    // LoanSet.cpp:674-682: no interest and no fees
    {TRANSACTION_LOAN_SET, STI_UINT32, XRP_UINT32_INTEREST_RATE, 0},
    {TRANSACTION_LOAN_SET, STI_UINT32, XRP_UINT32_LATE_INTEREST_RATE, 0},
    {TRANSACTION_LOAN_SET, STI_UINT32, XRP_UINT32_CLOSE_INTEREST_RATE, 0},
    {TRANSACTION_LOAN_SET, STI_UINT32, XRP_UINT32_OVERPAYMENT_INTEREST_RATE, 0},
    {TRANSACTION_LOAN_SET, STI_UINT32, XRP_UINT32_OVERPAYMENT_FEE, 0},
    {TRANSACTION_LOAN_SET, STI_NUMBER, XRP_NUMBER_LOAN_ORIGINATION_FEE, 0},
    {TRANSACTION_LOAN_SET, STI_NUMBER, XRP_NUMBER_LOAN_SERVICE_FEE, 0},
    {TRANSACTION_LOAN_SET, STI_NUMBER, XRP_NUMBER_LATE_PAYMENT_FEE, 0},
    {TRANSACTION_LOAN_SET, STI_NUMBER, XRP_NUMBER_CLOSE_PAYMENT_FEE, 0},
    // LoanSet.cpp:294: the loan broker's owner
    {TRANSACTION_LOAN_SET, STI_ACCOUNT, XRP_ACCOUNT_COUNTERPARTY, 0},
    // LoanBrokerSet.cpp:278-287, for a new broker only: no fee, no cover and
    // no debt limit
    {TRANSACTION_LOAN_BROKER_SET, STI_UINT16, XRP_UINT16_MANAGEMENT_FEE_RATE,
     0},
    {TRANSACTION_LOAN_BROKER_SET, STI_UINT32, XRP_UINT32_COVER_RATE_MINIMUM, 0},
    {TRANSACTION_LOAN_BROKER_SET, STI_UINT32, XRP_UINT32_COVER_RATE_LIQUIDATION,
     0},
    {TRANSACTION_LOAN_BROKER_SET, STI_NUMBER, XRP_NUMBER_DEBT_MAXIMUM, 0},
};
const size_t DEFAULT_FIELD_COUNT =
    sizeof(DEFAULT_FIELDS) / sizeof(DEFAULT_FIELDS[0]);

// A parsed field of these types always has bytes, so length 0 marks an
// added one.
bool is_default_field(const field_t* field, uint16_t transaction_type) {
    if (field->length != 0 || field->array_info.type != ARRAY_NONE) {
        return false;
    }
    for (size_t i = 0; i < DEFAULT_FIELD_COUNT; i++) {
        const default_field_t* d = &DEFAULT_FIELDS[i];
        if (d->transaction_type == transaction_type &&
            d->data_type == field->data_type && d->id == field->id) {
            return true;
        }
    }
    return false;
}

// A bit per transaction type from VaultCreate (65) on
#define TYPE_BIT(t) (1u << ((t) - TRANSACTION_VAULT_CREATE))

#define VAULT_CREATE TYPE_BIT(TRANSACTION_VAULT_CREATE)
#define VAULT_SET TYPE_BIT(TRANSACTION_VAULT_SET)
#define VAULT_DELETE TYPE_BIT(TRANSACTION_VAULT_DELETE)
#define VAULT_DEPOSIT TYPE_BIT(TRANSACTION_VAULT_DEPOSIT)
#define VAULT_WITHDRAW TYPE_BIT(TRANSACTION_VAULT_WITHDRAW)
#define VAULT_CLAWBACK TYPE_BIT(TRANSACTION_VAULT_CLAWBACK)
#define LOAN_BROKER_SET TYPE_BIT(TRANSACTION_LOAN_BROKER_SET)
#define LOAN_BROKER_DELETE TYPE_BIT(TRANSACTION_LOAN_BROKER_DELETE)
#define LOAN_BROKER_COVER_DEPOSIT \
    TYPE_BIT(TRANSACTION_LOAN_BROKER_COVER_DEPOSIT)
#define LOAN_BROKER_COVER_WITHDRAW \
    TYPE_BIT(TRANSACTION_LOAN_BROKER_COVER_WITHDRAW)
#define LOAN_BROKER_COVER_CLAWBACK \
    TYPE_BIT(TRANSACTION_LOAN_BROKER_COVER_CLAWBACK)
#define LOAN_SET TYPE_BIT(TRANSACTION_LOAN_SET)
#define LOAN_DELETE TYPE_BIT(TRANSACTION_LOAN_DELETE)
#define LOAN_MANAGE TYPE_BIT(TRANSACTION_LOAN_MANAGE)
#define LOAN_PAY TYPE_BIT(TRANSACTION_LOAN_PAY)

typedef struct {
    field_type_t data_type;
    uint8_t id;
    uint32_t types;
} scoped_field_t;

// The fields named for the vault and loan transactions, each with the types
// that carry it in rippled 3.4.1's transactions.macro. In any other transaction
// type they keep no name, as before these types were supported.
static const scoped_field_t SCOPED_FIELDS[] = {
    {STI_UINT8, XRP_UINT8_SCALE, VAULT_CREATE},
    {STI_UINT8, XRP_UINT8_WITHDRAWAL_POLICY, VAULT_CREATE},
    {STI_UINT8, XRP_UINT8_VAULT_KIND, VAULT_CREATE},
    {STI_UINT16, XRP_UINT16_MANAGEMENT_FEE_RATE, LOAN_BROKER_SET},
    {STI_UINT32, XRP_UINT32_PAYMENT_INTERVAL, LOAN_SET},
    {STI_UINT32, XRP_UINT32_GRACE_PERIOD, LOAN_SET},
    {STI_UINT32, XRP_UINT32_PAYMENT_TOTAL, LOAN_SET},
    {STI_UINT32, XRP_UINT32_COVER_RATE_MINIMUM, LOAN_BROKER_SET},
    {STI_UINT32, XRP_UINT32_COVER_RATE_LIQUIDATION, LOAN_BROKER_SET},
    {STI_UINT32, XRP_UINT32_OVERPAYMENT_FEE, LOAN_SET},
    {STI_UINT32, XRP_UINT32_INTEREST_RATE, LOAN_SET},
    {STI_UINT32, XRP_UINT32_LATE_INTEREST_RATE, LOAN_SET},
    {STI_UINT32, XRP_UINT32_CLOSE_INTEREST_RATE, LOAN_SET},
    {STI_UINT32, XRP_UINT32_OVERPAYMENT_INTEREST_RATE, LOAN_SET},
    {STI_UINT32, XRP_UINT32_SUBSCRIPTION_DATE, VAULT_CREATE},
    {STI_UINT32, XRP_UINT32_REDEMPTION_DATE, VAULT_CREATE},
    {STI_HASH256, XRP_HASH256_DOMAIN_ID, VAULT_CREATE | VAULT_SET},
    {STI_HASH256, XRP_HASH256_VAULT_ID,
     VAULT_SET | VAULT_DELETE | VAULT_DEPOSIT | VAULT_WITHDRAW |
         VAULT_CLAWBACK | LOAN_BROKER_SET},
    {STI_HASH256, XRP_HASH256_LOAN_BROKER_ID,
     LOAN_BROKER_SET | LOAN_BROKER_DELETE | LOAN_BROKER_COVER_DEPOSIT |
         LOAN_BROKER_COVER_WITHDRAW | LOAN_BROKER_COVER_CLAWBACK | LOAN_SET},
    {STI_HASH256, XRP_HASH256_LOAN_ID, LOAN_DELETE | LOAN_MANAGE | LOAN_PAY},
    {STI_VL, XRP_VL_DATA,
     VAULT_CREATE | VAULT_SET | LOAN_BROKER_SET | LOAN_SET},
    {STI_VL, XRP_VL_MPTOKEN_METADATA, VAULT_CREATE},
    {STI_ACCOUNT, XRP_ACCOUNT_HOLDER, VAULT_CLAWBACK},
    {STI_ACCOUNT, XRP_ACCOUNT_COUNTERPARTY, LOAN_SET},
    {STI_NUMBER, XRP_NUMBER_ASSETS_MAXIMUM, VAULT_CREATE | VAULT_SET},
    {STI_NUMBER, XRP_NUMBER_DEBT_MAXIMUM, LOAN_BROKER_SET},
    {STI_NUMBER, XRP_NUMBER_LOAN_ORIGINATION_FEE, LOAN_SET},
    {STI_NUMBER, XRP_NUMBER_LOAN_SERVICE_FEE, LOAN_SET},
    {STI_NUMBER, XRP_NUMBER_LATE_PAYMENT_FEE, LOAN_SET},
    {STI_NUMBER, XRP_NUMBER_CLOSE_PAYMENT_FEE, LOAN_SET},
    {STI_NUMBER, XRP_NUMBER_PRINCIPAL_REQUESTED, LOAN_SET},
};

static uint32_t type_bit(uint16_t transaction_type) {
    if (!is_vault_transaction(transaction_type) &&
        !is_loan_transaction(transaction_type)) {
        return 0;
    }
    return TYPE_BIT(transaction_type);
}

// Whether a field of SCOPED_FIELDS is in a type that carries it. Any other
// field is named in every type.
static bool is_field_in_scope(const field_t* field, uint16_t transaction_type) {
    for (size_t i = 0; i < sizeof(SCOPED_FIELDS) / sizeof(SCOPED_FIELDS[0]);
         i++) {
        const scoped_field_t* scoped = &SCOPED_FIELDS[i];
        if (field->data_type == scoped->data_type && field->id == scoped->id) {
            return (scoped->types & type_bit(transaction_type)) != 0;
        }
    }
    return true;
}

bool is_normal_account_field(field_t* field) {
    return field->data_type == STI_ACCOUNT &&
           field->id == XRP_ACCOUNT_ACCOUNT && field->array_info.type == 0;
}

// Returned by resolve_field_name() when no case below matches. Kept as one
// named constant so is_field_recognized() can identify that exact case by
// pointer, instead of duplicating this function's switch statements.
static const char UNKNOWN_FIELD_NAME[] = "Unknown";

const char* resolve_field_name(field_t* field) {
    if (!is_field_in_scope(field, parse_context.transaction_type)) {
        return UNKNOWN_FIELD_NAME;
    }

    if (field->data_type == STI_UINT16) {
        switch (field->id) {
            case 2:
                return "Transaction Type";
            case 3:
                return "Signer Weight";
            case 4:
                return "Transfer Fee";
            case 5:
                return "Trading Fee";
            case 6:
                return "Discount Fee";
            case 22:
                return "Management Fee Rate";
        }
    }

    if (field->data_type == STI_UINT32) {
        switch (field->id) {
            // 32-bit integers
            case 1:
                return "Network ID";
            case 2:
                return "Flags";
            case 3:
                return "Source Tag";
            case 4:
                return "Sequence";
            case 10:
                return "Expiration";
            case 11:
                return "Transfer Rate";
            case 12:
                return "Wallet Size";
            case 14:
                return "Destination Tag";
            case 20:
                return "Quality In";
            case 21:
                return "Quality Out";
            case 25:
                return "Offer Sequence";
            case 26:
                return "First Ledger Sequence";
            case 27:
                return "Last Ledger Sequence";
            case 29:
                return "Operation Limit";
            case 33:
                return "Set Flag";
            case 34:
                return "Clear Flag";
            case 35:
                return "Signer Quorum";
            case 36:
                return "Cancel After";
            case 37:
                return "Finish After";
            case 39:
                return "Settle Delay";
            case 40:
                return "Ticket Count";
            case 41:
                return "Ticket Sequence";
            case 42:
                return "NFToken Taxon";
            case 55:
                return "Payment Interval";
            case 56:
                return "Grace Period";
            case 60:
                return "Payment Total";
            case 62:
                return "Cover Rate Minimum";
            case 63:
                return "Cover Rate Liquidation";
            case 64:
                return "Overpayment Fee";
            case 65:
                return "Interest Rate";
            case 66:
                return "Late Interest Rate";
            case 67:
                return "Close Interest Rate";
            case 68:
                return "Overpayment Interest Rate";
            case 75:
                return "Subscription Date";
            case 76:
                return "Redemption Date";
        }
    }

    if (field->data_type == STI_HASH128) {
        switch (field->id) {
            case 1:
                return "Email Hash";
        }
    }

    if (field->data_type == STI_HASH256) {
        switch (field->id) {
            // 256-bit
            case 5:
                return "Previous Txn ID";
            case 7:
                return "Wallet Locator";
            case 9:
                return "Account Txn ID";
            case 10:
                return "NFToken ID";
            case 14:
                return "AMM ID";
            case 17:
                return "Invoice ID";
            case 21:
                return "Digest";
            case 22:
                return "Channel";
            case 24:
                return "Check ID";
            case 28:
                return "NFToken Buy Offer";
            case 29:
                return "NFToken Sell Offer";
            case 34:
                return "Domain ID";
            case 35:
                return "Vault ID";
            case 37:
                return "Loan Broker ID";
            case 38:
                return "Loan ID";
            case 0xFE:
                return "NFToken Offer";
        }
    }

    if (field->data_type == STI_AMOUNT) {
        switch (field->id) {
            // currency amount
            case 1:
                return "Amount";
            case 2:
                return "Balance";
            case 3:
                return "Limit Amount";
            case 4:
                return "Taker Pays";
            case 5:
                return "Taker Gets";
            case 8:
                return "Fee";
            case 9:
                return "Send Max";
            case 10:
                return "Deliver Min";
            case 11:
                return "Amount 2";
            case 12:
                return "Bid Min";
            case 13:
                return "Bid Max";
            case 19:
                return "NFToken Broker Fee";
            case 25:
                return "LP Token Out";
            case 26:
                return "LP Token In";
            case 27:
                return "E Price";
            case 28:
                return "Price";
            case 29:
                return "Signature Reward";
            case 30:
                return "Create Amount";
            case 31:
                return "LP Token Balance";
        }
    }

    if (field->data_type == STI_VL) {
        switch (field->id) {
            // variable length (common)
            case 1:
                return "Public Key";
            case 2:
                return "Message Key";
            case 3:
                return "Sig.PubKey";
            case 4:
                return "Txn Sig.";
            case 5:
                return "URI";
            case 6:
                return "Signature";
            case 7:
                return "Domain";
            case 12:
                return "Memo Type";
            case 13:
                return "Memo Data";
            case 14:
                return "Memo Fmt";
            case 16:
                return "Fulfillment";
            case 17:
                return "Condition";
            case 27:
                return "Data";
            case 30:
                return "Token Metadata";
        }
    }

    if (field->data_type == STI_VECTOR256) {
        switch (field->id) {
            // vector 256
            case 4:
                return "NFToken Offers";
        }
    }

    if (field->data_type == STI_ACCOUNT) {
        switch (field->id) {
            case 1:
                return "Account";
            case 2:
                return "Owner";
            case 3:
                return "Destination";
            case 4:
                return "Issuer";
            case 5:
                return "Authorize";
            case 6:
                return "Unauthorize";
            case 8:
                return "Regular Key";
            case 9:
                return "NFToken Minter";
            case 11:
                return "Holder";
            case 26:
                return "Counterparty";
        }
    }

    if (field->data_type == STI_OBJECT) {
        switch (field->id) {
            // inner object
            // OBJECT/1 is reserved for end of object
            case 10:
                return "Memo";
            case 11:
                return "Signer Entry";
            case 12:
                return "NFToken";
            case 16:
                return "Signer";
            case 27:
                return "Auth Account";
        }
    }

    if (field->data_type == STI_ARRAY) {
        switch (field->id) {
            // array of objects
            // ARRAY/1 is reserved for end of array
            case 3:
                return "Signers";
            case 4:
                return "Signer Entries";
            case 9:
                return "Memos";
            case 10:
                return "NFTokens";
            case 25:
                return "Auth Accounts";
        }
    }

    if (field->data_type == STI_UINT8) {
        switch (field->id) {
            // 8-bit integers
            case 4:
                return "Scale";
            case 16:
                return "Tick Size";
            case 20:
                return "Withdrawal Policy";
            case 22:
                return "Vault Kind";
        }
    }

    if (field->data_type == STI_NUMBER) {
        switch (field->id) {
            // numbers (XLS-65, XLS-66)
            case 3:
                return "Assets Maximum";
            case 7:
                return "Debt Maximum";
            case 9:
                return "Loan Origination Fee";
            case 10:
                return "Loan Service Fee";
            case 11:
                return "Late Payment Fee";
            case 12:
                return "Close Payment Fee";
            case 14:
                return "Principal Requested";
        }
    }

    if (field->data_type == STI_PATHSET) {
        switch (field->id) {
            case 1:
                return "Paths";
        }
    }

    if (field->data_type == STI_CURRENCY) {
        switch (field->id) {
            case 1:
                return "Currency";
        }
    }

    if (field->data_type == STI_ISSUE) {
        switch (field->id) {
            case 3:
                return "Asset";
            case 4:
                return "Asset 2";
        }
    }

    // Default case
    return UNKNOWN_FIELD_NAME;
}

// Whether the parser knows what this field means. A field the app cannot
// name is a field it cannot explain to the user during review, even
// though it is still part of the signed data.
bool is_field_recognized(field_t* field) {
    return resolve_field_name(field) != UNKNOWN_FIELD_NAME;
}

bool is_field_hidden(field_t* field) {
    HIDE(STI_UINT32, XRP_UINT32_SEQUENCE);
    HIDE(STI_UINT32, XRP_UINT32_LAST_LEDGER_SEQUENCE);
    HIDE(STI_VL, XRP_VL_SIGNING_PUB_KEY);

    if (field->data_type == STI_ARRAY || field->data_type == STI_OBJECT ||
        field->data_type == STI_PATHSET) {
        // Field is only used to instruct parsing code how to handle following
        // fields: don't show
        return true;
    }

    if (is_flag_hidden(field)) {
        return true;
    }

    return false;
}
