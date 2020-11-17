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
#include "idleMenu.h"
#include "../../glyphs.h"
#include <ux.h>

// clang-format off
UX_STEP_NOCB(
        ux_idle_flow_1_step,
        pnn,
        {
            &C_icon_XRP,
            "Use wallet to",
            "view accounts",
        });
UX_STEP_NOCB(
        ux_idle_flow_2_step,
        bn,
        {
            "Version",
            APPVERSION,
        });
UX_STEP_VALID(
        ux_idle_flow_3_step,
        pb,
        os_sched_exit(-1),
        {
            &C_icon_dashboard,
            "Quit",
        });
// clang-format on

UX_FLOW(ux_idle_flow, &ux_idle_flow_1_step, &ux_idle_flow_2_step, &ux_idle_flow_3_step);

void displayIdleMenu() {
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}
