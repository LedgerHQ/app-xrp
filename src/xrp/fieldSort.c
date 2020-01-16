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

#include "fieldSort.h"
#include "format/transactionTypes.h"

uint8_t getPriorityScore(field_t *field) {
    if (isTransactionTypeField(field)) {
        return 0;
    }

    if (isNormalAccountField(field)) {
        return 1;
    }

    return 2;
}

bool isPreceding(field_t *field, field_t *otherField) {
    return getPriorityScore(field) < getPriorityScore(otherField);
}

void swapFields(parseResult_t *result, uint8_t idx1, uint8_t idx2) {
    field_t bufferField;
    os_memcpy(&bufferField, &result->fields[idx1], sizeof(field_t));
    os_memcpy(&result->fields[idx1], &result->fields[idx2], sizeof(field_t));
    os_memcpy(&result->fields[idx2], &bufferField, sizeof(field_t));
}

void sortFields(parseResult_t *result) {
    for (uint8_t i = 0; i < result->numFields - 1; ++i) {
        field_t *curField = &result->fields[i];
        field_t *nextField = &result->fields[i + 1];

        if (isPreceding(nextField, curField) && !isPreceding(curField, nextField)) {
            swapFields(result, i, i + 1);
            i = MAX(0, i - 1) - 1; // Start next iteration at i - 1 or 0, whatever is largest
        }
    }
}
