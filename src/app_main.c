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

#include <stdint.h>  // uint*_t
#include <string.h>  // memset, explicit_bzero

// #include "os.h"
// #include "ux.h"
// #include "swap.h"

// // #include "types.h"
#include "io.h"
// #include "sw.h"
// #include "menu.h"
// #include "dispatcher.h"
// #include "dynamic_token_info.h"


#include <ux.h>

#include "address_ui.h"
#include "entry.h"
#include "global.h"
#include "globals.h"
#include "handle_check_address.h"
#include "handle_get_printable_amount.h"
#include "handle_swap_sign_transaction.h"
#include "idle_menu.h"
#include "swap.h"
// #include "os_io_seproxyhal.h"
// #include "swap_lib_calls.h"

const internal_storage_t N_storage_real;

/**
 * Handle APDU command received and send back APDU response using handlers.
 */
void app_main() {
    // Length of APDU command received in G_io_apdu_buffer
    int input_len = 0;

    io_init();


    // When called in swap context as a library, we don't want to show the menu
    if (!G_called_from_swap) {
        display_idle_menu();
    }

    for (;;) {
        // Receive command bytes in G_io_apdu_buffer
        if ((input_len = io_recv_command()) < 0) {
            PRINTF("=> io_recv_command failure\n");
            return;
        }

        // Parse APDU command from G_io_apdu_buffer
        
        if (handle_apdu() < 0) {
            PRINTF("=> FATAL handle_apdu failure\n");
            return;
        }
    }
}

// void app_main(void) {
//     volatile unsigned int rx = 0;
//     volatile unsigned int tx = 0;
//     volatile unsigned int flags = 0;

//     // Initialize the NVM data if required
//     if (N_storage.initialized != 0x01) {
//         internal_storage_t storage;
//         storage.allow_blind_sign = BlindSignDisabled;
//         storage.initialized = 0x01;
//         nvm_write((void*)&N_storage, &storage, sizeof(internal_storage_t));
//     }

//     // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
//     // goal is to retrieve APDU.
//     // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
//     // sure the io_event is called with a
//     // switch event, before the apdu is replied to the bootloader. This avoid
//     // APDU injection faults.
//     for (;;) {
//         volatile unsigned short sw = 0;

//         BEGIN_TRY {
//             TRY {
//                 rx = tx;
//                 tx = 0;  // ensure no race in catch_other if io_exchange throws
//                          // an error
//                 rx = io_exchange(CHANNEL_APDU | flags, rx);
//                 flags = 0;

//                 // no apdu received, well, reset the session, and reset the
//                 // bootloader configuration
//                 if (rx == 0) {
//                     THROW(0x6982);
//                 }

//                 PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);

//                 handle_apdu(&flags, &tx);
//             }
//             CATCH(EXCEPTION_IO_RESET) { THROW(EXCEPTION_IO_RESET); }
//             CATCH_OTHER(e) {
//                 switch (e & 0xF000u) {
//                     case 0x6000:
//                         // Wipe the transaction context and report the exception
//                         sw = e;
//                         reset_transaction_context();
//                         break;
//                     case 0x9000:
//                         // All is well
//                         sw = e;
//                         break;
//                     default:
//                         // Internal error
//                         sw = 0x6800u | (e & 0x7FFu);
//                         reset_transaction_context();
//                         break;
//                 }
//                 // Unexpected exception => report
//                 G_io_apdu_buffer[tx] = sw >> 8u;
//                 G_io_apdu_buffer[tx + 1] = sw;
//                 tx += 2;
//             }
//             FINALLY {}
//         }
//         END_TRY;
//     }
// }