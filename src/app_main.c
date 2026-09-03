/*****************************************************************************
 *   Ledger App Boilerplate.
 *   (c) 2020 Ledger SAS.
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
 *****************************************************************************/

#include <status_words.h>
#include <stdint.h>  // uint*_t
#include <string.h>  // memset, explicit_bzero
#include <ux.h>

#include "address_ui.h"
#include "constants.h"
#include "entry.h"
#include "global.h"
#include "globals.h"
#include "idle_menu.h"
#include "io.h"
#include "parser.h"
#include "swap.h"

const internal_storage_t N_storage_real;

/**
 * Handle APDU command received and send back APDU response using handlers.
 */
void app_main() {
    io_init();

    // When called in swap context as a library, we don't want to show the menu
    if (!G_called_from_swap) {
        display_idle_menu();
    }

    for (;;) {
        // Receive command bytes in G_io_apdu_buffer
        int rx_len = io_recv_command();
        if (rx_len < 0) {
            PRINTF("=> io_recv_command failure\n");
            return;
        }

        // Reject a truncated APDU so handlers never read stale
        // G_io_apdu_buffer bytes as if they were part of this command.
        // The rx_len check covers an edge case apdu_parser() alone would
        // accept even though handle_apdu() still reads
        // G_io_apdu_buffer[OFFSET_LC] directly.
        command_t cmd;
        if (!apdu_parser(&cmd, G_io_apdu_buffer, (size_t)rx_len) ||
            (size_t)rx_len < OFFSET_CDATA) {
            reset_transaction_context();
            io_send_sw(SWO_WRONG_DATA_LENGTH);
            continue;
        }

        // Parse APDU command from G_io_apdu_buffer
        if (handle_apdu() < 0) {
            PRINTF("=> FATAL handle_apdu failure\n");
            return;
        }
    }
}
