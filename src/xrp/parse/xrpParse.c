/*******************************************************************************
 *   XRP Wallet
 *   (c) 2017 Ledger
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

#include "xrpParse.h"
#include "../xrpHelpers.h"
#include "../format/amount.h"
#include "../format/array.h"
#include "../format/fields.h"
#include "../format/readers.h"
#include "../format/transactionTypes.h"
#include "../fieldSort.h"
#include "../format/flags.h"

#define SUCCESS 0
#define CHECK(x)                  \
    do {                          \
        err = x;                  \
        if (err.err != SUCCESS) { \
            return err;           \
        }                         \
    } while (0)

bool hasData(parseContext_t *context, uint32_t numBytes) {
    return context->offset + numBytes - 1 < context->length;
}

bool hasField(parseContext_t *context, uint8_t dataType, uint8_t id) {
    for (uint8_t i = 0; i < context->result.numFields; ++i) {
        field_t *field = &context->result.fields[i];
        if (field->dataType == dataType && field->id == id) {
            return true;
        }
    }

    return false;
}

uint8_t *currentPosition(parseContext_t *context) {
    return context->data + context->offset;
}

typedef struct err_s {
    int err;
} err_t;

err_t advancePosition(parseContext_t *context, uint32_t numBytes) {
    err_t err;

    if (hasData(context, numBytes)) {
        context->offset += numBytes;
        err.err = SUCCESS;
    } else {
        err.err = EXCEPTION_OVERFLOW;
    }

    return err;
}

err_t readNextByte(parseContext_t *context, uint8_t *result) {
    err_t err;

    if (hasData(context, 1)) {
        *result = *currentPosition(context);
        err = advancePosition(context, 1);
    } else {
        err.err = EXCEPTION_OVERFLOW;
    }

    return err;
}

err_t peakNextByte(parseContext_t *context, uint8_t *result) {
    err_t err;

    if (hasData(context, 1)) {
        *result = *currentPosition(context);
        err.err = SUCCESS;
    } else {
        err.err = EXCEPTION_OVERFLOW;
    }

    return err;
}

void appendArrayInfo(parseContext_t *context, field_t *field) {
    if (context->currentArray != 0) {
        field->arrayInfo.type = context->currentArray;
        field->arrayInfo.index1 = context->arrayIndex1;
        field->arrayInfo.index2 = context->arrayIndex2;
    }
}

err_t appendNewField(parseContext_t *context, field_t **field) {
    err_t err;

    if (context->result.numFields >= MAX_FIELD_COUNT) {
        err.err = NOT_ENOUGH_SPACE;
        return err;
    }

    *field = &context->result.fields[context->result.numFields++];
    appendArrayInfo(context, *field);

    err.err = SUCCESS;
    return err;
}

err_t removeLastField(parseContext_t *context, field_t *lastField) {
    err_t err;

    if (context->result.numFields == 0) {
        err.err = INVALID_STATE;
        return err;
    }

    // Verify that the last field in the array corresponds to the field that we
    // are supposed to remove.
    if (&context->result.fields[context->result.numFields - 1] != lastField) {
        err.err = INVALID_STATE;
        return err;
    }

    // Make sure that all data is cleared so that we don't end up with
    // an unexpected state later on.
    memset(lastField, 0, sizeof(field_t));
    context->result.numFields--;

    err.err = SUCCESS;
    return err;
}

err_t readFixedSizeField(parseContext_t *context, field_t *field, uint16_t length) {
    field->data = currentPosition(context);
    field->length = length;

    return advancePosition(context, length);
}

err_t readVariableLengthField(parseContext_t *context, field_t *field) {
    uint8_t value;
    err_t err;

    CHECK(readNextByte(context, &value));

    uint16_t dataLength = value;
    if (dataLength > 192) {
        if (dataLength > 240) {
            // It is impossible to send a transaction large enough to
            // hold a field with a length of 12481 or greater, so we
            // should never reach this point with a valid transaction
            err.err = INVALID_STATE;
            return err;
        }

        uint8_t lengthByte2;
        CHECK(readNextByte(context, &lengthByte2));
        dataLength = 193 + ((dataLength - 193) * 256) + lengthByte2;
    }

    return readFixedSizeField(context, field, dataLength);
}

err_t readAmount(parseContext_t *context, field_t *field) {
    uint8_t firstByte;
    err_t err;

    CHECK(peakNextByte(context, &firstByte));
    if ((firstByte >> 7u) == 0) {
        CHECK(readFixedSizeField(context, field, 8));
    } else {
        CHECK(readFixedSizeField(context, field, 48));

        if (hasNonStandardCurrency(field)) {
            field_t *currency;
            CHECK(appendNewField(context, &currency));
            currency->dataType = STI_CURRENCY;
            currency->id = XRP_CURRENCY_CURRENCY;
            currency->data = field->data + 8;
            currency->length = 20;
        }

        field_t *issuer;
        CHECK(appendNewField(context, &issuer));
        issuer->dataType = STI_ACCOUNT;
        issuer->id = XRP_ACCOUNT_ISSUER;
        issuer->data = field->data + 28;
        issuer->length = 20;
    }

    return err;
}

void handleArrayField(parseContext_t *context, field_t *field) {
    if (field->id != ARR_END) {
        // Begin array
        context->currentArray = field->id;
        context->arrayIndex1 = 0;
        context->arrayIndex2 = 0;
    } else {
        // End array
        context->currentArray = ARRAY_NONE;
    }
}

err_t handleObjectField(parseContext_t *context, field_t *field) {
    err_t err;

    if (field->id != OBJ_END) {
        // Normal object, only increment first array index
        context->arrayIndex1++;

        // Explicitly limit the maximum number of array items
        if (context->arrayIndex1 > MAX_ARRAY_LEN) {
            err.err = NOT_SUPPORTED;
            return err;
        }
    }

    err.err = SUCCESS;
    return err;
}

err_t handlePathStep(parseContext_t *context, field_t *field, uint8_t stepType) {
    field_t *new_field;
    err_t err;

    CHECK(readFixedSizeField(context, field, 20));

    switch (stepType) {
        case 0x01:
            field->dataType = STI_ACCOUNT;
            field->id = XRP_ACCOUNT_ACCOUNT;
            break;
        case 0x20:
            field->dataType = STI_ACCOUNT;
            field->id = XRP_ACCOUNT_ISSUER;
            break;
        case 0x30:
            // Read the issuer separately
            CHECK(appendNewField(context, &new_field));
            CHECK(handlePathStep(context, new_field, 0x20));

            // Intentional fall through to read the currency,
            // which will be positioned before the issuer
        case 0x10:
            field->dataType = STI_CURRENCY;
            field->id = XRP_CURRENCY_CURRENCY;
            break;
        default:
            field->dataType = STI_PATHSET;
            break;
    }

    return err;
}

err_t handlePathField(parseContext_t *context, field_t *field) {
    // Set default type to STI_PATHSET, which is hidden
    field->dataType = STI_PATHSET;

    uint8_t currentStep;
    err_t err;
    CHECK(readNextByte(context, &currentStep));
    switch (currentStep) {
        case PATHSET_NEXT:
            // Indicator for next item. Increase index and continue parsing
            context->arrayIndex1++;
            context->arrayIndex2 = 1;

            // Limit the number of paths to the specified maximum
            if (context->arrayIndex1 > MAX_PATH_COUNT) {
                err.err = INVALID_STATE;
                return err;
            }

            break;
        case PATHSET_END:
            // End of path set, stop parsing
            context->currentArray = ARRAY_NONE;
            break;
        default:
            // Verify that the step count is within specified bounds before
            // processing the step data
            if (context->arrayIndex2 > MAX_STEP_COUNT) {
                err.err = INVALID_STATE;
                return err;
            }

            // Actual path step found, perform specific parsing
            CHECK(handlePathStep(context, field, currentStep));

            // The array index is incremented here because handlePathStep
            // may append more than one field with the same index
            context->arrayIndex2++;
            break;
    }

    return err;
}

void handlePathSetField(parseContext_t *context) {
    context->currentArray = ARRAY_PATHSET;

    // The code for handling path set fields becomes easier if we
    // begin the arrays at one, which is what we want to present
    // to the user
    context->arrayIndex1 = 1;
    context->arrayIndex2 = 1;
}

err_t readFieldValue(parseContext_t *context, field_t *field) {
    err_t err;
    err.err = SUCCESS;

    switch (field->dataType) {
        case STI_UINT8:
            err = readFixedSizeField(context, field, 1);
            break;
        case STI_UINT16:
            err = readFixedSizeField(context, field, 2);
            break;
        case STI_UINT32:
            err = readFixedSizeField(context, field, 4);
            break;
        case STI_HASH128:
            err = readFixedSizeField(context, field, 16);
            break;
        case STI_HASH256:
            err = readFixedSizeField(context, field, 32);
            break;
        case STI_AMOUNT:
            err = readAmount(context, field);
            break;
        case STI_VL:
            // Intentional fall-through
        case STI_ACCOUNT:
            err = readVariableLengthField(context, field);
            break;
        case STI_ARRAY:
            handleArrayField(context, field);
            break;
        case STI_OBJECT:
            err = handleObjectField(context, field);
            break;
        case STI_PATHSET:
            handlePathSetField(context);
            break;
        default:
            err.err = NOT_SUPPORTED;
            break;
    }

    return err;
}

err_t readFieldHeader(parseContext_t *context, field_t *field) {
    uint8_t firstByte;
    err_t err;

    CHECK(readNextByte(context, &firstByte));

    if (firstByte >> 4u == 0) {
        // Type code >= 16
        CHECK(readNextByte(context, &field->dataType));
        if (firstByte == 0) {
            // Field code >= 16
            CHECK(readNextByte(context, &field->id));
        } else {
            // Field code < 16
            field->id = firstByte & 0x0fu;
        }
    } else {
        // Type code < 16
        field->dataType = firstByte >> 4u;
        if ((firstByte & 0x0fu) == 0) {
            // Field code >= 16
            CHECK(readNextByte(context, &field->id));
        } else {
            // Field code < 16
            field->id = firstByte & 0x0fu;
        }
    }

    return err;
}

err_t postProcessField(parseContext_t *context, field_t *field) {
    err_t err;
    err.err = SUCCESS;

    switch (field->dataType) {
        case STI_UINT16:
            // Record the transaction type since it must be available for the
            // formatting of certain values
            if (isTransactionTypeField(field)) {
                context->transactionType = readUnsigned16(field->data);
            }

            break;
        case STI_UINT32:
            // Reject transaction if tfFullyCanonicalSig is not set
            if (field->id == XRP_UINT32_FLAGS) {
                uint32_t value = readUnsigned32(field->data);
                if ((value & TF_FULLY_CANONICAL_SIG) == 0) {
                    err.err = 0x6800;
                    return err;
                }
            }

            break;
        case STI_VL:
            // Detect when SigningPubKey is empty (needed for multi-sign)
            if (field->id == XRP_VL_SIGNING_PUB_KEY && field->length == 0) {
                context->hasEmptyPubKey = true;
            }
            break;
        case STI_ACCOUNT:
            // Safety check to capture the illegal case where an account
            // field is not 20 bytes long.
            if (field->length != 20) {
                err.err = INVALID_STATE;
                return err;
            }
            break;
    }

    return err;
}

err_t postProcessTransaction(parseContext_t *context) {
    err_t err;
    err.err = SUCCESS;

    // Append "empty" regular key field when clearing it
    if (context->transactionType == TRANSACTION_SET_REGULAR_KEY &&
        !hasField(context, STI_ACCOUNT, 8)) {
        field_t *field;
        CHECK(appendNewField(context, &field));
        field->dataType = STI_ACCOUNT;
        field->id = XRP_ACCOUNT_REGULAR_KEY;
        field->data = NULL;  // Special value to indicate empty regular key
    }

    return err;
}

err_t readField(parseContext_t *context, field_t *field) {
    err_t err;

    if (context->currentArray == ARRAY_PATHSET) {
        CHECK(handlePathField(context, field));
    } else {
        CHECK(readFieldHeader(context, field));
        CHECK(readFieldValue(context, field));
    }

    return postProcessField(context, field);
}

err_t parseTxInternal(parseContext_t *context) {
    err_t err;

    context->transactionType = TRANSACTION_INVALID;
    context->hasEmptyPubKey = false;

    while (context->offset != context->length) {
        if (context->offset > context->length) {
            err.err = EXCEPTION_OVERFLOW;
            return err;
        }

        field_t *field;
        CHECK(appendNewField(context, &field));
        CHECK(readField(context, field));

        if (isFieldHidden(field)) {
            // This must be done after all data has been read since we
            // always need to keep track of the input stream position.
            CHECK(removeLastField(context, field));
        }
    }

    CHECK(postProcessTransaction(context));
    sortFields(&context->result);

    err.err = SUCCESS;
    return err;
}

int parseTx(parseContext_t *context) {
    err_t err = parseTxInternal(context);
    return err.err;
}
