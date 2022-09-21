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
#include <os.h>
#include <os_io_seproxyhal.h>
#include <string.h>
#include <ux.h>
#include "global.h"
#include "transaction.h"
#include "format.h"
#include "idle_menu.h"
#include "review_menu.h"
#include "nbgl_page.h"
#include "nbgl_use_case.h"

#define MAX_FIELDS_PER_PAGE 5

// Globals
static field_value_t txFieldValueStrings[MAX_FIELDS_PER_PAGE];
static nbgl_layoutTagValue_t pair;
static nbgl_layoutTagValueList_t pairList;
static nbgl_pageInfoLongPress_t infoLongPress;
static parseResult_t *transaction;
static resultAction_t approval_menu_callback;

// Static functions declarations
static nbgl_layoutTagValue_t *getPair(uint8_t index);
static void reviewStart(void);
static void displayTransaction(void);
static void reviewChoice(bool confirm);
static void rejectConfirmation(void);
static void rejectChoice(void);

// Static functions definitions
static void reviewStart(void) {
    // Reset globals
    memset(&infoLongPress, 0, sizeof(infoLongPress));
    memset(&txFieldValueStrings, 0, sizeof(txFieldValueStrings));
    memset(&pair, 0, sizeof(pair));

    infoLongPress.text = "Confirm Transaction";
    infoLongPress.longPressText = "Hold to confirm";
    infoLongPress.icon = &C_icon_XRP_64px;

    pairList.pairs = NULL;
    pairList.nbPairs = transaction->num_fields;
    pairList.nbMaxLinesForValue = 0;
    pairList.callback = getPair;
    pairList.startIndex = 0;

    nbgl_useCaseReviewStart(&C_icon_XRP_64px,
                            "Review transaction",
                            NULL,
                            "Reject",
                            displayTransaction,
                            rejectChoice);
}

// function called by NBGL to get the pair indexed by "index"
static nbgl_layoutTagValue_t *getPair(uint8_t index) {
    uint8_t arr_idx = index % MAX_FIELDS_PER_PAGE;
    memset(&txFieldValueStrings[arr_idx], 0, sizeof(field_value_t));
    // Format tag item string.
    pair.item = (char *) resolve_field_name(&transaction->fields[index]);
    // Format tag value string.
    format_field(&transaction->fields[index], &txFieldValueStrings[arr_idx]);
    pair.value = txFieldValueStrings[arr_idx].buf;
    PRINTF("Arr idx %d - Tag %d item : %s\nTag %d value : %s\n",
           arr_idx,
           index,
           pair.item,
           index,
           pair.value);
    return &pair;
}

static void displayTransaction(void) {
    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject", reviewChoice);
}

static void reviewChoice(bool confirm) {
    if (confirm) {
        nbgl_useCaseStatus("TRANSACTION\nCONFIRMED", true, display_idle_menu);
        approval_menu_callback(OPTION_SIGN);
    } else {
        rejectChoice();
    }
}

static void rejectConfirmation(void) {
    nbgl_useCaseStatus("Transaction rejected", false, display_idle_menu);
    approval_menu_callback(OPTION_REJECT);
}

static void rejectChoice(void) {
    nbgl_useCaseConfirm("Reject transaction?",
                        NULL,
                        "Yes, Reject",
                        "Go back to transaction",
                        rejectConfirmation);
}

void display_review_menu(parseResult_t *transaction_param, resultAction_t callback) {
    transaction = transaction_param;
    approval_menu_callback = callback;
    // Prepare and display transaction
    reviewStart();
}
#endif  // HAVE_NBGL
