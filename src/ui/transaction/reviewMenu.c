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

#include "reviewMenu.h"
#include <os.h>
#include <os_io_seproxyhal.h>
#include <string.h>
#include "../../transaction/transaction.h"
#include "../../xrp/format/format.h"

char fieldName[MAX_FIELDNAME_LEN];
char fieldValue[MAX_FIELD_LEN];

parseResult_t *transaction;
action_t nextAction;

const ux_flow_step_t* ux_review_flow[MAX_FIELD_COUNT + 1];

void updateContent(int stackSlot);

UX_STEP_CB_INIT(
        ux_review_flow_step,
        bnnn_paging,
        updateContent(stack_slot),
        nextAction(),
        {
            fieldName,
            fieldValue
        });

void updateTitle(field_t *field) {
    memset(fieldName, 0, MAX_FIELDNAME_LEN);
    resolveFieldName(field, fieldName);

    if (field->arrayInfo.type == ARRAY_PATHSET) {
        SNPRINTF(fieldName + strlen(fieldName), " [P%d: S%d]", field->arrayInfo.index1, field->arrayInfo.index2);
    } else if (field->arrayInfo.type != ARRAY_NONE) {
        SNPRINTF(fieldName + strlen(fieldName), " [%d]", field->arrayInfo.index1);
    }
}

void updateValue(field_t *field) {
    formatField(field, fieldValue);
}

void updateContent(int stackSlot) {
    int stepIndex = G_ux.flow_stack[stackSlot].index;
    field_t *field = &transaction->fields[stepIndex];

    updateTitle(field);
    updateValue(field);
}

void displayReviewMenu(parseResult_t *transactionParam, action_t next) {
    transaction = transactionParam;
    nextAction = next;

    for (int i = 0; i < transaction->numFields; ++i) {
        ux_review_flow[i] = &ux_review_flow_step;
    }
    ux_review_flow[transaction->numFields] = FLOW_END_STEP;

    ux_flow_init(0, ux_review_flow, NULL);
}
