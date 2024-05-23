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
static const char* const infoTypes[] = {"Version", "Developer"};
static const char* const infoContents[] = {APPVERSION, "Ledger"};

static void on_quit_clbk(void) {
    os_sched_exit(-1);
}

void display_idle_menu() {
    static nbgl_contentInfoList_t infosList = {0};

    infosList.nbInfos = NB_INFO_FIELDS;
    infosList.infoTypes = (const char**) infoTypes;
    infosList.infoContents = (const char**) infoContents;
    nbgl_useCaseHomeAndSettings(APPNAME,
                                &C_icon_XRP_64px,
                                NULL,
                                INIT_HOME_PAGE,
                                NULL,
                                &infosList,
                                NULL,
                                on_quit_clbk);
}
#endif  // HAVE_NBGL
