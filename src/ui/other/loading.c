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

#include "loading.h"
#include <os.h>
#include <os_io_seproxyhal.h>
#include "../ux.h"
#include <string.h>

/**
 * This file implements a loading screen that can be displayed while performing
 * a time-consuming action. The implementation has been designed to be as
 * efficient as possible while maintaining a reliable function.
 *
 * In essence, the loading screen implementation consists of three steps after
 * its appearance has been requested:
 *
 * 1. Request the display of loadingUI. This UI has a dummy last element that
 *    will never be displayed. The reason for this is that interrupts to the
 *    rendering of the UI (such as calling a time-consuming function) will
 *    cause the element that is currently rendering to either disappear or
 *    flicker.
 *
 * 2. Once the dummy element is displayed, a timer is set to re-render the UI
 *    as soon as possible. It is not possible to execute the time-consuming
 *    action here because the UI has not yet been shown in its complete form.
 *
 * 3. The second time the UI is rendered the loading text is clearly visible
 *    on the display, and the time-consuming function can thus be executed.
 *    To avoid that the preprocessor executes this function again, a state
 *    enum is set before the function is executed. This is done because the
 *    UI might be rendered a third time before the function finishes
 *    executing.
 *
 * Note: Since this loading screen implementation is dependent on functionality
 * in the BAGL UI system it has not been rewritten using Flow UI.
 */

#define STATE_WAITING 0
#define STATE_READY   1
#define STATE_DONE    2

#define UID_DUMMY 1

static uint8_t loadingState;
static action_t pendingAction;
static char loadingMessage[18];

static const bagl_element_t loadingUI[] = {UI_BACKGROUND(),
                                           UI_SINGLE_TEXT(loadingMessage),
                                           UI_DUMMY(UID_DUMMY)};

static unsigned int loadingUI_button(unsigned int button_mask, unsigned int button_mask_counter) {
    // It is not possible to omit this function
    UNUSED(button_mask);
    UNUSED(button_mask_counter);

    return 0;
}

static const bagl_element_t* loadingUI_button_prepro(const bagl_element_t* element) {
    if (element->component.userid == UID_DUMMY) {
        if (loadingState == STATE_WAITING) {
            loadingState = STATE_READY;
            UX_CALLBACK_SET_INTERVAL(1)
        } else if (loadingState == STATE_READY) {
            loadingState = STATE_DONE;

            pendingAction();
            pendingAction = NULL;
        }

        return NULL;
    } else {
        return element;
    }
}

void executeAsync(action_t actionToLoad, char* message) {
    loadingState = STATE_WAITING;
    pendingAction = actionToLoad;

    os_memset(loadingMessage, 0, sizeof(loadingMessage));
    os_memmove(loadingMessage, message, MIN(sizeof(loadingMessage) - 1, strlen(message)));

    UX_DISPLAY(loadingUI, loadingUI_button_prepro)
}
