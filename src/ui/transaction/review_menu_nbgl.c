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
#include <ux.h>

#include "fmt.h"
#include "idle_menu.h"
#include "nbgl_use_case.h"
#include "review_menu.h"
#include "ui.h"

#define MAX_FIELDS_PER_PAGE 8

// Globals
static field_value_t txFieldValueStrings[MAX_FIELDS_PER_PAGE];
static field_name_t txFieldNameStrings[MAX_FIELDS_PER_PAGE];
static nbgl_contentTagValue_t pair;
static nbgl_contentTagValueList_t pairList;
static parseResult_t* transaction;
static resultAction_t approval_menu_callback;

static void build_title(const field_t* field, field_name_t* title) {
    const char* name = resolve_field_name((field_t*)field);
    strncpy(title->buf, name, sizeof(title->buf));
    title->buf[sizeof(title->buf) - 1] = '\0';
    size_t len = strlen(title->buf);
    if (field->array_info.type == ARRAY_PATHSET) {
        snprintf(title->buf + len, sizeof(title->buf) - len, " [P%d: S%d]",
                 field->array_info.index1, field->array_info.index2);
    } else if (field->array_info.type != ARRAY_NONE) {
        snprintf(title->buf + len, sizeof(title->buf) - len, " [%d]",
                 field->array_info.index1);
    }
}

// function called by NBGL to get the pair indexed by "index"
static nbgl_layoutTagValue_t* getPair(uint8_t index) {
    uint8_t arr_idx = index % MAX_FIELDS_PER_PAGE;
    memset(&txFieldValueStrings[arr_idx], 0, sizeof(field_value_t));
    memset(&txFieldNameStrings[arr_idx], 0, sizeof(field_name_t));
    build_title(&transaction->fields[index], &txFieldNameStrings[arr_idx]);
    pair.item = txFieldNameStrings[arr_idx].buf;
    format_field(&transaction->fields[index], &txFieldValueStrings[arr_idx]);
    pair.value = txFieldValueStrings[arr_idx].buf;
    PRINTF("Arr idx %d - Tag %d item : %s\nTag %d value : %s\n", arr_idx, index,
           pair.item, index, pair.value);
    return &pair;
}

static void reviewChoice(bool confirm) {
    if (confirm) {
        approval_menu_callback(OPTION_SIGN);
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_SIGNED,
                                 display_idle_menu);
    } else {
        approval_menu_callback(OPTION_REJECT);
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_REJECTED,
                                 display_idle_menu);
    }
}

void display_blind_signed_review(parseResult_t* transaction_param,
                                 resultAction_t callback) {
    transaction = transaction_param;
    approval_menu_callback = callback;

    memset(&txFieldValueStrings, 0, sizeof(txFieldValueStrings));
    memset(&txFieldNameStrings, 0, sizeof(txFieldNameStrings));
    memset(&pair, 0, sizeof(pair));

    pairList.pairs = NULL;
    pairList.nbPairs = transaction->num_fields;
    pairList.nbMaxLinesForValue = 0;
    pairList.callback = getPair;
    pairList.startIndex = 0;

    nbgl_useCaseReviewBlindSigning(TYPE_TRANSACTION, &pairList, &ICON_APP_HOME,
                                   "Review transaction", NULL,
                                   "Sign transaction?", NULL, reviewChoice);
}

void display_review_menu(parseResult_t* transaction_param,
                         resultAction_t callback) {
    transaction = transaction_param;
    approval_menu_callback = callback;

    // Reset globals
    memset(&txFieldValueStrings, 0, sizeof(txFieldValueStrings));
    memset(&txFieldNameStrings, 0, sizeof(txFieldNameStrings));
    memset(&pair, 0, sizeof(pair));

    pairList.pairs = NULL;
    pairList.nbPairs = transaction->num_fields;
    pairList.nbMaxLinesForValue = 0;
    pairList.callback = getPair;
    pairList.startIndex = 0;

    nbgl_useCaseReview(TYPE_TRANSACTION, &pairList, &ICON_APP_HOME,
                       "Review transaction", NULL, "Sign transaction?",
                       reviewChoice);
}
