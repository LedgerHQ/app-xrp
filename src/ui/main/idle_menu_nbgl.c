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
#include <ux.h>
#include "glyphs.h"
#include "idle_menu.h"
#include "nbgl_use_case.h"

#define NB_INFO_FIELDS 2
static const char* const infoTypes[] = {"Version", "XRP App"};
static const char* const infoContents[] = {APPVERSION, "(c) 2023 Ledger"};

static void display_about_menu();
static bool about_nav_clbk(uint8_t page, nbgl_pageContent_t* content);
static void on_quit_clbk(void);

static bool about_nav_clbk(uint8_t page, nbgl_pageContent_t* content) {
    if (page == 0) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = NB_INFO_FIELDS;
        content->infosList.infoTypes = (const char**) infoTypes;
        content->infosList.infoContents = (const char**) infoContents;
    } else {
        return false;
    }
    return true;
}

static void display_about_menu(void) {
    nbgl_useCaseSettings("App infos", 0, 1, true, display_idle_menu, about_nav_clbk, NULL);
}

static void on_quit_clbk(void) {
    os_sched_exit(-1);
}

void display_idle_menu() {
    nbgl_useCaseHome("XRP",
                     &C_icon_XRP_64px,
                     "This app confirms actions on\nthe XRP network.",
                     true,
                     display_about_menu,
                     on_quit_clbk);
}
#endif  // HAVE_NBGL
