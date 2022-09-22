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

#ifdef HAVE_NBGL

#include "nbgl_page.h"

enum {
    BACK_TOKEN = 0,
    NEXT_TOKEN,
    CANCEL_TOKEN,
    VALIDATE_TRANSACTION_TOKEN,
    START_REVIEW_TOKEN,
    CANCEL_REJECT_TOKEN,
    CONFIRM_REJECT_TOKEN
};

extern const nbgl_icon_details_t C_icon_XRP_32px;
extern const nbgl_icon_details_t C_round_warning_64px;
extern nbgl_page_t *pageContext;
static nbgl_layoutTagValue_t tagValues[MAX_FIELD_COUNT];
static int8_t pageIdx;
static uint8_t nbPages;
static int8_t pairsIdx;

extern void releaseContext(void);
static void pageTouchCallback(int token, uint8_t index);

static void tickerCallback(void) {
    releaseContext();
    approval_menu_callback(OPTION_SIGN);
}

static void displayTransactionPage(void) {
    if (pageIdx == -1) {  // Introduction page
        nbgl_pageInfoDescription_t info = {.centeredInfo.icon = &C_icon_XRP_32px,
                                           .centeredInfo.text1 = "Review transaction",
                                           .centeredInfo.text2 = NULL,
                                           .centeredInfo.text3 = NULL,
                                           .centeredInfo.style = LARGE_CASE_INFO,
                                           .centeredInfo.offsetY = -32,
                                           .footerText = "Reject",
                                           .footerToken = CANCEL_TOKEN,
                                           .tapActionText = "Tap to continue",
                                           .tapActionToken = START_REVIEW_TOKEN,
                                           .topRightStyle = NO_BUTTON_STYLE,
                                           .tuneId = TUNE_TAP_CASUAL};

        releaseContext();
        pageContext = nbgl_pageDrawInfo(&pageTouchCallback, NULL, &info);
    }
    if (pageIdx >= 0 && pageIdx < nbPages) {  // Transaction pages (up to the last).
        // Only display a back button if we are not on the first transaction page
        bool displayBackButton = (pageIdx > 0) ? true : false;
        // If this is the last transaction page and the number of fields is not a multiple of 4, only display the remaining fields.
        uint8_t nbPairs = (pageIdx == nbPages - 1 && transaction->num_fields % 4 != 0) ? transaction->num_fields % 4 : 4;
        
        PRINTF("Pair idx : %d, tx page num : %d, pairs to display : %d\n",pairsIdx,pageIdx,nbPairs);
        
        nbgl_pageNavigationInfo_t info = {.activePage = pageIdx,
                                          .nbPages = nbPages + 1,
                                          .navType = NAV_WITH_TAP,
                                          .progressIndicator = true,
                                          .navWithTap.backButton = displayBackButton,
                                          .navWithTap.backToken = BACK_TOKEN,
                                          .navWithTap.nextPageText = "Tap to continue",
                                          .navWithTap.nextPageToken = NEXT_TOKEN,
                                          .navWithTap.quitText = "Reject",
                                          .quitToken = CANCEL_TOKEN,
                                          .tuneId = TUNE_TAP_CASUAL};
        nbgl_pageContent_t content = {.type = TAG_VALUE_LIST,
                                      .tagValueList.nbPairs = nbPairs,
                                      .tagValueList.pairs = (nbgl_layoutTagValue_t *) (&tagValues[pairsIdx])};
        releaseContext();
        pageContext = nbgl_pageDrawGenericContent(&pageTouchCallback, &info, &content);
    } else if (pageIdx == nbPages) {  // Last page of transaction
        nbgl_pageNavigationInfo_t info = {.activePage = pageIdx,
                                          .nbPages = nbPages + 1,
                                          .navType = NAV_WITH_TAP,
                                          .progressIndicator = true,
                                          .navWithTap.backButton = true,
                                          .navWithTap.backToken = BACK_TOKEN,
                                          .navWithTap.nextPageText = NULL,
                                          .navWithTap.quitText = "Reject",
                                          .quitToken = CANCEL_TOKEN,
                                          .tuneId = TUNE_TAP_CASUAL};
        nbgl_pageContent_t content = {
            .type = INFO_LONG_PRESS,
            .infoLongPress.centeredInfo.icon = &C_icon_XRP_32px,
            .infoLongPress.centeredInfo.text1 = "Confirm transaction",
            .infoLongPress.centeredInfo.text2 = NULL,
            .infoLongPress.centeredInfo.text3 = NULL,
            .infoLongPress.centeredInfo.style = LARGE_CASE_INFO,
            .infoLongPress.centeredInfo.offsetY = -32,
            .infoLongPress.longPressText = "Hold to confirm",
            .infoLongPress.longPressToken = VALIDATE_TRANSACTION_TOKEN,
            .infoLongPress.tuneId = TUNE_TAP_NEXT};
        releaseContext();
        pageContext = nbgl_pageDrawGenericContent(&pageTouchCallback, &info, &content);
    }
    PRINTF("REFRESH\n");
    nbgl_refresh();
}

static void pageTouchCallback(int token, uint8_t index) {
    (void) index;
    PRINTF("Enter touch callback. (token : %d)\n",token);
    switch(token) {
        case BACK_TOKEN:
            if (pageIdx >= 0) {
                pageIdx--;
                pairsIdx-=4;
                displayTransactionPage();
            }
            break;
        case NEXT_TOKEN:
            if (pageIdx < nbPages) {
                pageIdx++;
                pairsIdx+=4;
                displayTransactionPage();
            }    
            break;
        case CANCEL_TOKEN:
            nbgl_pageConfirmationDescription_t info = {
                .cancelToken = CANCEL_REJECT_TOKEN,
                .centeredInfo.text1 = "Reject transaction?",
                .centeredInfo.text2 = NULL,
                .centeredInfo.text3 = NULL,
                .centeredInfo.style = LARGE_CASE_INFO,
                .centeredInfo.icon = &C_round_warning_64px,
                .centeredInfo.offsetY = -64,
                .confirmationText = "Reject",
                .confirmationToken = CONFIRM_REJECT_TOKEN,
                .tuneId = TUNE_TAP_NEXT};
            releaseContext();
            pageContext = nbgl_pageDrawConfirmation(&pageTouchCallback, &info);
            nbgl_refresh();
            break;
        case VALIDATE_TRANSACTION_TOKEN:
            nbgl_screenTickerConfiguration_t ticker = {.tickerCallback = &tickerCallback,
                                                       .tickerIntervale = 1000,
                                                       .tickerValue = 2000};
            releaseContext();
            pageContext =
                nbgl_pageDrawLedgerInfo(&pageTouchCallback, &ticker, "TRANSACTION\nCONFIRMED");
            nbgl_refresh();
            break;
        case START_REVIEW_TOKEN:
            pageIdx=0;
            pairsIdx=0;
            displayTransactionPage();
            break;
        case CANCEL_REJECT_TOKEN:
            displayTransactionPage();
            break;
        case CONFIRM_REJECT_TOKEN:
            approval_menu_callback(OPTION_REJECT);
            break;
        default:
            PRINTF("Should not happen !\n");
            break;
    }
}

#endif  // HAVE_NBGL

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
#ifdef HAVE_NBGL
    // Prepare data for transaction pages
    field_value_t tmp_values[transaction->num_fields];

    for (int i = 0; i < transaction->num_fields; ++i) {
        tagValues[i].item = (char*) resolve_field_name(&transaction->fields[i]);
        format_field(&transaction->fields[i], &tmp_values[i]);
        tagValues[i].value = tmp_values[i].buf;
    }

    nbPages = transaction->num_fields / 4;

    // If number of fields is not a multiple of 4 but larger than 4, add a page
    // to display the remaining fields.
    // Limit the number of fields to 4 to be sure we can display all of them.
    if(transaction->num_fields % 4 != 0 && nbPages != 0)
    {
        nbPages++;
    }
    // Otherwise if the number of fields is smaller than 4, just display one page.
    else if(nbPages == 0)
    {
        nbPages = 1;
    }

    // Start with the introduction page
    pageIdx = -1;
    // Display it
    displayTransactionPage();
#endif  // HAVE_NBGL
}
