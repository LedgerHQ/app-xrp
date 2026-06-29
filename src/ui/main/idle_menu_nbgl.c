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
#include <os_io_seproxyhal.h>
#include <ux.h>

#include "globals.h"
#include "glyphs.h"
#include "idle_menu.h"
#include "nbgl_use_case.h"
#include "os.h"
#include "ui.h"

//  -----------------------------------------------------------
//  --------------------- SETTINGS MENU -----------------------
//  -----------------------------------------------------------
#define SETTING_INFO_NB 2
static const char* const INFO_TYPES[SETTING_INFO_NB] = {"Version", "Developer"};
static const char* const INFO_CONTENTS[SETTING_INFO_NB] = {APPVERSION,
                                                           "Ledger"};

static const nbgl_contentInfoList_t infoList = {
    .nbInfos = SETTING_INFO_NB,
    .infoTypes = INFO_TYPES,
    .infoContents = INFO_CONTENTS,
};

enum {
    BLIND_SIGNING_IDX = 0,
    NB_SETTINGS_SWITCHES,
};
static nbgl_layoutSwitch_t G_switches[NB_SETTINGS_SWITCHES];

enum {
    BLIND_SIGNING_TOKEN = FIRST_USER_TOKEN,
};

static void settings_controls_callback(int token, uint8_t index, int page);

// settings menu definition
#define SETTING_CONTENTS_NB 1
static const nbgl_content_t contents[SETTING_CONTENTS_NB] = {
    {.type = SWITCHES_LIST,
     .content.switchesList.nbSwitches = NB_SETTINGS_SWITCHES,
     .content.switchesList.switches = G_switches,
     .contentActionCallback = settings_controls_callback}};

static const nbgl_genericContents_t settingContents = {
    .callbackCallNeeded = false,
    .contentsList = contents,
    .nbContents = SETTING_CONTENTS_NB};

static void settings_controls_callback(int token, uint8_t index, int page) {
    (void)index;
    (void)page;
    uint8_t new_setting;
    switch (token) {
        case BLIND_SIGNING_TOKEN:
            // Write in NVM the opposite of what the current toggle is
            new_setting = (G_switches[BLIND_SIGNING_IDX].initState != ON_STATE);
            G_switches[BLIND_SIGNING_IDX].initState = (nbgl_state_t)new_setting;
            nvm_write((void*)&N_storage.allow_blind_sign, &new_setting,
                      sizeof(new_setting));
            break;
        default:
            PRINTF("Unreachable\n");
            break;
    }
}

void set_switches_states(void) {
    G_switches[BLIND_SIGNING_IDX].text = "Blind signing";
    G_switches[BLIND_SIGNING_IDX].subText = "Enable blind signing";
    G_switches[BLIND_SIGNING_IDX].token = BLIND_SIGNING_TOKEN;
#ifdef HAVE_PIEZO_SOUND
    G_switches[BLIND_SIGNING_IDX].tuneId = TUNE_TAP_CASUAL;
#endif
    if (N_storage.allow_blind_sign == BlindSignDisabled) {
        G_switches[BLIND_SIGNING_IDX].initState = OFF_STATE;
    } else {
        G_switches[BLIND_SIGNING_IDX].initState = ON_STATE;
    }
}

static void on_quit_clbk(void) { os_sched_exit(-1); }

// home page definition
void display_idle_menu(void) {
    set_switches_states();
    nbgl_useCaseHomeAndSettings(APPNAME, &ICON_APP_HOME, NULL, INIT_HOME_PAGE,
                                &settingContents, &infoList, NULL,
                                on_quit_clbk);
}

void display_settings_menu(void) {
    set_switches_states();
    nbgl_useCaseHomeAndSettings(APPNAME, &ICON_APP_HOME, NULL, 0,
                                &settingContents, &infoList, NULL,
                                on_quit_clbk);
}
