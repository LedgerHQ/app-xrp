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
#include "../../apdu/global.h"
#include "../../transaction/transaction.h"
#include "../../xrp/format/format.h"
#include "../../glyphs.h"

parseResult_t *transaction;
resultAction_t approvalMenuCallback;

const ux_flow_step_t *ux_review_flow[MAX_FIELD_COUNT + 3];

void updateContent(int stackSlot);

// clang-format off
UX_STEP_NOCB_INIT(
        ux_review_flow_step,
        bnnn_paging,
        updateContent(stack_slot),
        {
            approvalStrings.review.fieldName,
            approvalStrings.review.fieldValue
        });

UX_STEP_VALID(
        ux_review_flow_sign,
        pn,
        approvalMenuCallback(OPTION_SIGN),
        {
            &C_icon_validate_14,
            "Sign transaction"
        });

UX_STEP_VALID(
        ux_review_flow_reject,
        pn,
        approvalMenuCallback(OPTION_REJECT),
        {
            &C_icon_crossmark,
            "Reject",
        });
// clang-format on

void updateTitle(field_t *field, char *title) {
    memset(title, 0, MAX_FIELDNAME_LEN);
    resolveFieldName(field, title);

    if (field->arrayInfo.type == ARRAY_PATHSET) {
        SNPRINTF(title + strlen(title),
                 " [P%d: S%d]",
                 field->arrayInfo.index1,
                 field->arrayInfo.index2);
    } else if (field->arrayInfo.type != ARRAY_NONE) {
        SNPRINTF(title + strlen(title), " [%d]", field->arrayInfo.index1);
    }
}

void updateValue(field_t *field, char *value) {
    formatField(field, value);
}

void updateContent(int stackSlot) {
    int stepIndex = G_ux.flow_stack[stackSlot].index;
    field_t *field = &transaction->fields[stepIndex];

    updateTitle(field, approvalStrings.review.fieldName);
    updateValue(field, approvalStrings.review.fieldValue);
}

void displayReviewMenu(parseResult_t *transactionParam, resultAction_t callback) {
    transaction = transactionParam;
    approvalMenuCallback = callback;

    for (int i = 0; i < transaction->numFields; ++i) {
        ux_review_flow[i] = &ux_review_flow_step;
    }

    ux_review_flow[transaction->numFields + 0] = &ux_review_flow_sign;
    ux_review_flow[transaction->numFields + 1] = &ux_review_flow_reject;
    ux_review_flow[transaction->numFields + 2] = FLOW_END_STEP;

    ux_flow_init(0, ux_review_flow, NULL);
}
