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

#include "review_menu.h"
#include <os.h>
#include <os_io_seproxyhal.h>
#include <string.h>
#include <ux.h>
#include "../../apdu/global.h"
#include "../../transaction/transaction.h"
#include "../../xrp/format.h"

parseResult_t *transaction;
resultAction_t approval_menu_callback;

#ifdef HAVE_BAGL

static void update_content(int stack_slot);
const ux_flow_step_t *ux_review_flow[MAX_FIELD_COUNT + 3];

// clang-format off
UX_STEP_NOCB_INIT(
        ux_review_flow_step,
        bnnn_paging,
        update_content(stack_slot),
        {
            approval_strings.review.field_name.buf,
            approval_strings.review.field_value.buf
        });

UX_STEP_CB(
        ux_review_flow_sign,
        pn,
        approval_menu_callback(OPTION_SIGN),
        {
            &C_icon_validate_14,
            "Sign transaction"
        });

UX_STEP_CB(
        ux_review_flow_reject,
        pn,
        approval_menu_callback(OPTION_REJECT),
        {
            &C_icon_crossmark,
            "Reject",
        });
// clang-format on

static void update_title(field_t *field, field_name_t *title) {
    const char *name = resolve_field_name(field);
    strncpy(title->buf, name, sizeof(title->buf));
    title->buf[sizeof(title->buf) - 1] = '\x00';

    size_t len = strlen(title->buf);
    if (field->array_info.type == ARRAY_PATHSET) {
        snprintf(title->buf + len,
                 sizeof(title->buf) - len,
                 " [P%d: S%d]",
                 field->array_info.index1,
                 field->array_info.index2);
    } else if (field->array_info.type != ARRAY_NONE) {
        snprintf(title->buf + len, sizeof(title->buf) - len, " [%d]", field->array_info.index1);
    }
}

static void update_value(field_t *field, field_value_t *value) {
    format_field(field, value);
}

static void update_content(int stack_slot) {
    int step_index = G_ux.flow_stack[stack_slot].index;
    field_t *field = &transaction->fields[step_index];

    update_title(field, &approval_strings.review.field_name);
    update_value(field, &approval_strings.review.field_value);
}
#endif //HAVE_BAGL

void display_review_menu(parseResult_t *transaction_param, resultAction_t callback) {
    transaction = transaction_param;
    approval_menu_callback = callback;

#ifdef HAVE_BAGL
    for (int i = 0; i < transaction->num_fields; ++i) {
        ux_review_flow[i] = &ux_review_flow_step;
    }

    ux_review_flow[transaction->num_fields + 0] = &ux_review_flow_sign;
    ux_review_flow[transaction->num_fields + 1] = &ux_review_flow_reject;
    ux_review_flow[transaction->num_fields + 2] = FLOW_END_STEP;

    ux_flow_init(0, ux_review_flow, NULL);
#endif //HAVE_BAGL
}
