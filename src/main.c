/*******************************************************************************
 *   XRP Wallet
 *   (c) 2017 Ledger
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

#include "os_io_seproxyhal.h"
#include "apdu/entry.h"
#include "apdu/global.h"
#include "ui/main/idle_menu.h"
#include "ui/address/address_ui.h"
#include <ux.h>

#include "swap/swap_lib_calls.h"
#include "swap/handle_swap_sign_transaction.h"
#include "swap/handle_get_printable_amount.h"
#include "swap/handle_check_address.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD:
            break;

        // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
        case CHANNEL_SPI:
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }
                return 0;  // nothing received from the master so far (it's a tx
                           // transaction)
            } else {
                return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
            }

        default:
            THROW(INVALID_PARAMETER);
    }
    return 0;
}

void app_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0;  // ensure no race in catch_other if io_exchange throws
                         // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);

                handle_apdu(&flags, &tx);
            }
            CATCH(EXCEPTION_IO_RESET) {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000u) {
                    case 0x6000:
                        // Wipe the transaction context and report the exception
                        sw = e;
                        reset_transaction_context();
                        break;
                    case 0x9000:
                        // All is well
                        sw = e;
                        break;
                    default:
                        // Internal error
                        sw = 0x6800u | (e & 0x7FFu);
                        reset_transaction_context();
                        break;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8u;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
}

#ifdef HAVE_BAGL
// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}
#endif  // HAVE_BAGL

void handle_seproxyhal_tag_finger_event() {
    UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
}

#ifdef HAVE_BAGL
void handle_seproxyhal_tag_button_push_event() {
    UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
}
#endif  // HAVE_BAGL

void handle_seproxyhal_tag_status_event() {
    if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
        !(U4BE(G_io_seproxyhal_spi_buffer, 3) & SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
        THROW(EXCEPTION_IO_RESET);
    }
}

void handle_default() {
    UX_DEFAULT_EVENT();
}

#ifdef HAVE_BAGL
void handle_seproxyhal_tag_display_processed_event() {
    UX_DISPLAYED_EVENT({});
}
#endif  // HAVE_BAGL

void handle_seproxyhal_tag_ticker_event() {
    UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
        if (UX_ALLOWED) {
            // redisplay screen
            UX_REDISPLAY();
        }
    });
}

unsigned char io_event(unsigned char channel) {
    UNUSED(channel);

    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
#ifdef HAVE_BAGL
            handle_seproxyhal_tag_button_push_event();
#endif  // HAVE_BAGL
            break;

        case SEPROXYHAL_TAG_STATUS_EVENT:
            handle_seproxyhal_tag_status_event();
        // no break is intentional
        default:
            handle_default();
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
#ifdef HAVE_BAGL
            handle_seproxyhal_tag_display_processed_event();
#endif  // HAVE_BAGL
            break;
#ifdef HAVE_NBGL
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;
#endif  // HAVE_NBGL
        case SEPROXYHAL_TAG_TICKER_EVENT:
            handle_seproxyhal_tag_ticker_event();
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

void app_exit(void) {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}

void coin_main() {
    for (;;) {
        called_from_swap = false;
        reset_transaction_context();

        UX_INIT();

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

#ifdef TARGET_NANOX
                // grab the current plane mode setting
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif  // TARGET_NANOX

                USB_power(0);
                USB_power(1);

                display_idle_menu();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif  // HAVE_BLE

                app_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                // reset IO and UX before continuing
                CLOSE_TRY;
                continue;
            }
            CATCH_ALL {
                CLOSE_TRY;
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
    app_exit();
}

struct libargs_s {
    unsigned int id;
    unsigned int command;
    unsigned int unused;
    union {
        check_address_parameters_t *check_address;
        create_transaction_parameters_t *create_transaction;
        get_printable_amount_parameters_t *get_printable_amount;
    };
};

static void library_main_helper(struct libargs_s *args) {
    check_api_level(CX_COMPAT_APILEVEL);
    PRINTF("Inside a library \n");
    switch (args->command) {
        case CHECK_ADDRESS:
            // ensure result is zero if an exception is thrown
            args->check_address->result = 0;
            args->check_address->result = handle_check_address(args->check_address);
            break;
        case SIGN_TRANSACTION:
            if (copy_transaction_parameters(args->create_transaction)) {
                // never returns
                handle_swap_sign_transaction();
            }
            break;
        case GET_PRINTABLE_AMOUNT:
            // ensure result is zero if an exception is thrown (compatibility breaking, disabled
            // until LL is ready)
            // args->get_printable_amount->result = 0;
            // args->get_printable_amount->result =
            handle_get_printable_amount(args->get_printable_amount);
            break;
        default:
            break;
    }
}

void library_main(struct libargs_s *args) {
    bool end = false;
    /* This loop ensures that library_main_helper and os_lib_end are called
     * within a try context, even if an exception is thrown */
    while (1) {
        BEGIN_TRY {
            TRY {
                if (!end) {
                    library_main_helper(args);
                }
                os_lib_end();
            }
            FINALLY {
                end = true;
            }
        }
        END_TRY;
    }
}

__attribute__((section(".boot"))) int main(int arg0) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

    if (arg0 == 0) {
        // called from dashboard as standalone xrp app
        coin_main();
    } else {
        // Called as library from another app
        struct libargs_s *args = (struct libargs_s *) arg0;
        if (args->id == 0x100) {
            library_main(args);
        } else {
            app_exit();
        }
    }

    return 0;
}
