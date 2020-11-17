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

#include "xrpParse.h"
#include "../xrpHelpers.h"
#include "string.h"
#include "../format/amount.h"
#include "../format/array.h"
#include "../format/fields.h"
#include "../format/readers.h"
#include "../format/transactionTypes.h"
#include "../fieldSort.h"
#include "../format/flags.h"

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

void advancePosition(parseContext_t *context, uint32_t numBytes) {
    if (hasData(context, numBytes)) {
        context->offset += numBytes;
    } else {
        THROW(EXCEPTION_OVERFLOW);
    }
}

uint8_t readNextByte(parseContext_t *context) {
    if (hasData(context, 1)) {
        uint8_t value = *currentPosition(context);
        advancePosition(context, 1);

        return value;
    } else {
        THROW(EXCEPTION_OVERFLOW);
    }
}

uint8_t peakNextByte(parseContext_t *context) {
    if (hasData(context, 1)) {
        return *currentPosition(context);
    } else {
        THROW(EXCEPTION_OVERFLOW);
    }
}

void appendArrayInfo(parseContext_t *context, field_t *field) {
    if (context->currentArray != 0) {
        field->arrayInfo.type = context->currentArray;
        field->arrayInfo.index1 = context->arrayIndex1;
        field->arrayInfo.index2 = context->arrayIndex2;
    }
}

field_t *appendNewField(parseContext_t *context) {
    if (context->result.numFields >= MAX_FIELD_COUNT) {
        THROW(NOT_ENOUGH_SPACE);
    }

    field_t *field = &context->result.fields[context->result.numFields++];
    appendArrayInfo(context, field);

    return field;
}

void removeLastField(parseContext_t *context, field_t *lastField) {
    if (context->result.numFields == 0) {
        THROW(INVALID_STATE);
    }

    // Verify that the last field in the array corresponds to the field that we
    // are supposed to remove.
    if (&context->result.fields[context->result.numFields - 1] != lastField) {
        THROW(INVALID_STATE);
    }

    // Make sure that all data is cleared so that we don't end up with
    // an unexpected state later on.
    memset(lastField, 0, sizeof(field_t));
    context->result.numFields--;
}

void readFixedSizeField(parseContext_t *context, field_t *field, uint16_t length) {
    field->data = currentPosition(context);
    field->length = length;

    advancePosition(context, length);
}

void readVariableLengthField(parseContext_t *context, field_t *field) {
    uint16_t dataLength = readNextByte(context);
    if (dataLength > 192) {
        if (dataLength > 240) {
            // It is impossible to send a transaction large enough to
            // hold a field with a length of 12481 or greater, so we
            // should never reach this point with a valid transaction
            THROW(INVALID_STATE);
        }

        uint8_t lengthByte2 = readNextByte(context);
        dataLength = 193 + ((dataLength - 193) * 256) + lengthByte2;
    }

    readFixedSizeField(context, field, dataLength);
}

void readAmount(parseContext_t *context, field_t *field) {
    uint8_t firstByte = peakNextByte(context);

    if ((firstByte >> 7u) == 0) {
        readFixedSizeField(context, field, 8);
    } else {
        readFixedSizeField(context, field, 48);

        if (hasNonStandardCurrency(field)) {
            field_t *currency = appendNewField(context);
            currency->dataType = STI_CURRENCY;
            currency->id = XRP_CURRENCY_CURRENCY;
            currency->data = field->data + 8;
            currency->length = 20;
        }

        field_t *issuer = appendNewField(context);
        issuer->dataType = STI_ACCOUNT;
        issuer->id = XRP_ACCOUNT_ISSUER;
        issuer->data = field->data + 28;
        issuer->length = 20;
    }
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

void handleObjectField(parseContext_t *context, field_t *field) {
    if (field->id != OBJ_END) {
        // Normal object, only increment first array index
        context->arrayIndex1++;

        // Explicitly limit the maximum number of array items
        if (context->arrayIndex1 > MAX_ARRAY_LEN) {
            THROW(NOT_SUPPORTED);
        }
    }
}

void handlePathStep(parseContext_t *context, field_t *field, uint8_t stepType) {
    readFixedSizeField(context, field, 20);

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
            handlePathStep(context, appendNewField(context), 0x20);

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
}

void handlePathField(parseContext_t *context, field_t *field) {
    // Set default type to STI_PATHSET, which is hidden
    field->dataType = STI_PATHSET;

    uint8_t currentStep = readNextByte(context);
    switch (currentStep) {
        case PATHSET_NEXT:
            // Indicator for next item. Increase index and continue parsing
            context->arrayIndex1++;
            context->arrayIndex2 = 1;

            // Limit the number of paths to the specified maximum
            if (context->arrayIndex1 > MAX_PATH_COUNT) {
                THROW(INVALID_STATE);
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
                THROW(INVALID_STATE);
            }

            // Actual path step found, perform specific parsing
            handlePathStep(context, field, currentStep);

            // The array index is incremented here because handlePathStep
            // may append more than one field with the same index
            context->arrayIndex2++;
            break;
    }
}

void handlePathSetField(parseContext_t *context) {
    context->currentArray = ARRAY_PATHSET;

    // The code for handling path set fields becomes easier if we
    // begin the arrays at one, which is what we want to present
    // to the user
    context->arrayIndex1 = 1;
    context->arrayIndex2 = 1;
}

void readFieldValue(parseContext_t *context, field_t *field) {
    switch (field->dataType) {
        case STI_UINT8:
            readFixedSizeField(context, field, 1);
            break;
        case STI_UINT16:
            readFixedSizeField(context, field, 2);
            break;
        case STI_UINT32:
            readFixedSizeField(context, field, 4);
            break;
        case STI_HASH128:
            readFixedSizeField(context, field, 16);
            break;
        case STI_HASH256:
            readFixedSizeField(context, field, 32);
            break;
        case STI_AMOUNT:
            readAmount(context, field);
            break;
        case STI_VL:
            // Intentional fall-through
        case STI_ACCOUNT:
            readVariableLengthField(context, field);
            break;
        case STI_ARRAY:
            handleArrayField(context, field);
            break;
        case STI_OBJECT:
            handleObjectField(context, field);
            break;
        case STI_PATHSET:
            handlePathSetField(context);
            break;
        default:
            THROW(NOT_SUPPORTED);
    }
}

void readFieldHeader(parseContext_t *context, field_t *field) {
    uint8_t firstByte = readNextByte(context);
    if (firstByte >> 4u == 0) {
        // Type code >= 16
        field->dataType = readNextByte(context);

        if (firstByte == 0) {
            // Field code >= 16
            field->id = readNextByte(context);
        } else {
            // Field code < 16
            field->id = firstByte & 0x0fu;
        }
    } else {
        // Type code < 16
        field->dataType = firstByte >> 4u;

        if ((firstByte & 0x0fu) == 0) {
            // Field code >= 16
            field->id = readNextByte(context);
        } else {
            // Field code < 16
            field->id = firstByte & 0x0fu;
        }
    }
}

void postProcessField(parseContext_t *context, field_t *field) {
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
                    THROW(0x6800);
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
                THROW(INVALID_STATE);
            }
            break;
    }
}

void postProcessTransaction(parseContext_t *context) {
    // Append "empty" regular key field when clearing it
    if (context->transactionType == TRANSACTION_SET_REGULAR_KEY &&
        !hasField(context, STI_ACCOUNT, 8)) {
        field_t *field = appendNewField(context);
        field->dataType = STI_ACCOUNT;
        field->id = XRP_ACCOUNT_REGULAR_KEY;
        field->data = NULL;  // Special value to indicate empty regular key
    }
}

void readField(parseContext_t *context, field_t *field) {
    if (context->currentArray == ARRAY_PATHSET) {
        handlePathField(context, field);
    } else {
        readFieldHeader(context, field);
        readFieldValue(context, field);
    }

    postProcessField(context, field);
}

void parseTxInternal(parseContext_t *context) {
    context->transactionType = TRANSACTION_INVALID;
    context->hasEmptyPubKey = false;

    while (context->offset != context->length) {
        if (context->offset > context->length) {
            THROW(EXCEPTION_OVERFLOW);
        }

        field_t *field = appendNewField(context);
        readField(context, field);

        if (isFieldHidden(field)) {
            // This must be done after all data has been read since we
            // always need to keep track of the input stream position.
            removeLastField(context, field);
        }
    }

    postProcessTransaction(context);
    sortFields(&context->result);
}

void parseTx(parseContext_t *context) {
    BEGIN_TRY {
        TRY {
            parseTxInternal(context);
        }
        CATCH_OTHER(e) {
            switch (e & 0xF000u) {
                case 0x6000:
                    // Proper error, forward it further
                    THROW(e);
                default:
                    // Mask real cause behind generic error (INCORRECT_DATA)
                    THROW(0x6A80);
            }
        }
        FINALLY {
        }
    }
    END_TRY
}
