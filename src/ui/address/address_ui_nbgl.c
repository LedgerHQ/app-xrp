/*******************************************************************************
 *   XRP Wallet
 *   (c) 2022 Ledger
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
#ifdef HAVE_NBGL
#include <os_io_seproxyhal.h>
#include <string.h>
#include <ux.h>
#include "address_ui.h"
#include "idle_menu.h"
#include "nbgl_page.h"
#include "nbgl_use_case.h"

static char full_address[43];
static action_t approval_action;
static action_t rejection_action;

static void confirmationChoiceClbk(bool confirm) {
    if (confirm) {
        approval_action();
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_VERIFIED, display_idle_menu);
    } else {
        rejection_action();
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_REJECTED, display_idle_menu);
    }
}

void display_address_confirmation_ui(char* address, action_t on_approve, action_t on_reject) {
    approval_action = on_approve;
    rejection_action = on_reject;
    memset(full_address, 0, sizeof(full_address));
    strncpy(full_address, address, sizeof(full_address));
    nbgl_useCaseAddressReview((char*) full_address,
                              NULL,
                              &C_icon_XRP_64px,
                              "Verify XRP Address",
                              NULL,
                              confirmationChoiceClbk);
}
#endif  // HAVE_NBGL
