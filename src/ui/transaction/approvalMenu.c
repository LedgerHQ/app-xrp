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

#include "approvalMenu.h"
#include <os_io_seproxyhal.h>
#include "../../glyphs.h"

resultAction_t approvalMenuCallback;

UX_STEP_VALID(
        ux_approval_flow_1_step,
        pn,
        approvalMenuCallback(OPTION_SIGN),
        {
            &C_icon_validate_14,
            "Sign transaction"
        });
UX_STEP_VALID(
        ux_approval_flow_2_step,
        pn,
        approvalMenuCallback(OPTION_REVIEW),
        {
            &C_icon_eye,
            "Preview again"
        });
UX_STEP_VALID(
        ux_approval_flow_3_step,
        pn,
        approvalMenuCallback(OPTION_REJECT),
        {
            &C_icon_crossmark,
            "Reject",
        });

const ux_flow_step_t *        const ux_approval_flow [] = {
        &ux_approval_flow_1_step,
        &ux_approval_flow_2_step,
        &ux_approval_flow_3_step,
        FLOW_END_STEP,
};

void displayApprovalMenu(resultAction_t callback) {
    approvalMenuCallback = callback;

    ux_flow_init(0, ux_approval_flow, NULL);
}
