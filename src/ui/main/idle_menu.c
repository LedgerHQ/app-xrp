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
#include "idle_menu.h"
#include "../../glyphs.h"
#include <ux.h>

#ifdef HAVE_NBGL

#include "nbgl_page.h"

static void display_about_menu();
static void pageTouchCallback(int token, uint8_t index);

static const char* const infoTypes[] = {"Version", "XRP App"};
static const char* const infoContents[] = {APPVERSION, "(c) 2022 Ledger"};
nbgl_page_t* pageContext;

void releaseContext(void) {
    if (pageContext != NULL) {
        nbgl_pageRelease(pageContext);
        pageContext = NULL;
    }
}

enum { BACK_TOKEN = 0, INFO_TOKEN, NEXT_TOKEN, CANCEL_TOKEN, QUIT_INFO_TOKEN, QUIT_APP_TOKEN };


static void pageTouchCallback(int token, uint8_t index) {
    (void) index;
    if (token == QUIT_APP_TOKEN) {
        releaseContext();
        os_sched_exit(-1);
    } else if (token == INFO_TOKEN) {
        display_about_menu();
    } else if (token == QUIT_INFO_TOKEN) {
        releaseContext();
        display_idle_menu();
    }
}

static void display_about_menu(void) {
    nbgl_pageContent_t content = {.title = "App info", .isTouchableTitle = false};
    nbgl_pageNavigationInfo_t nav = {.activePage = 0,
                                     .nbPages = 1,
                                     .navType = NAV_WITH_BUTTONS,
                                     .navWithButtons.quitButton = true,
                                     .navWithButtons.navToken = QUIT_INFO_TOKEN,
                                     .tuneId = TUNE_TAP_CASUAL};
    content.type = INFOS_LIST;
    content.infosList.nbInfos = 2;
    content.infosList.infoTypes = (const char**) infoTypes;
    content.infosList.infoContents = (const char**) infoContents;

    releaseContext();
    pageContext = nbgl_pageDrawGenericContent(&pageTouchCallback, &nav, &content);
    nbgl_refresh();
}

#endif  // HAVE_NBGL

#ifdef HAVE_BAGL
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
UX_STEP_CB(
        ux_idle_flow_3_step,
        pb,
        os_sched_exit(-1),
        {
            &C_icon_dashboard,
            "Quit",
        });
// clang-format on

UX_FLOW(ux_idle_flow, &ux_idle_flow_1_step, &ux_idle_flow_2_step, &ux_idle_flow_3_step);
#endif //HAVE_BAGL

void display_idle_menu() {
#ifdef HAVE_BAGL
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
#endif //HAVE_BAGL
#ifdef HAVE_NBGL
    nbgl_pageInfoDescription_t info = {
        .centeredInfo.icon = &C_icon_XRP_32px,
        .centeredInfo.text1 = "XRP",
        .centeredInfo.text2 =
            "Go to Ledger Live to create a\ntransaction. You will approve it\non Stax.",
        .centeredInfo.text3 = NULL,
        .centeredInfo.style = LARGE_CASE_INFO,
        .centeredInfo.offsetY = 32,
        .footerText = NULL,
        .bottomButtonStyle = QUIT_ICON,
        .bottomButtonToken = QUIT_APP_TOKEN,
        .tapActionText = NULL,
        .topRightStyle = INFO_ICON,
        .topRightToken = INFO_TOKEN,
        .tuneId = TUNE_TAP_CASUAL};

    releaseContext();
    pageContext = nbgl_pageDrawInfo(&pageTouchCallback, NULL, &info);
    nbgl_refresh();
#endif  // HAVE_NBGL
}

