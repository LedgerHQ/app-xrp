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

#include <os_io_seproxyhal.h>
#include <string.h>
#include "../../glyphs.h"
#include "address_ui.h"

static char full_address[43];
static action_t approval_action;
static action_t rejection_action;

// clang-format off
UX_STEP_NOCB(
        ux_display_address_flow_1_step,
        pnn,
        {
            &C_icon_eye,
            "Verify",
            "address",
        });
UX_STEP_NOCB(
        ux_display_address_flow_2_step,
        bnnn_paging,
        {
            "Address",
            full_address,
        });
UX_STEP_VALID(
        ux_display_address_flow_3_step,
        pb,
        approval_action(),
        {
            &C_icon_validate_14,
            "Approve",
        });
UX_STEP_VALID(
        ux_display_address_flow_4_step,
        pb,
        rejection_action(),
        {
            &C_icon_crossmark,
            "Reject",
        });
// clang-format on

UX_FLOW(ux_display_address_flow,
        &ux_display_address_flow_1_step,
        &ux_display_address_flow_2_step,
        &ux_display_address_flow_3_step,
        &ux_display_address_flow_4_step);

void display_address_confirmation_ui(char* address, action_t on_approve, action_t on_reject) {
    approval_action = on_approve;
    rejection_action = on_reject;

    memset(full_address, 0, sizeof(full_address));
    strncpy(full_address, address, sizeof(full_address));

    ux_flow_init(0, ux_display_address_flow, NULL);
}
