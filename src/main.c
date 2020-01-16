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
#include "ui/main/idleMenu.h"
#include "ui/address/addressUI.h"
#include <ux.h>

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

#define COLOR_APP 0x27a2db

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
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

void sample_main(void) {
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
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);

                handleApdu(&flags, &tx);
            }
            CATCH(EXCEPTION_IO_RESET){
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000u) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    resetTransactionContext();
                    break;
                case 0x9000:
                    // All is well
                    sw = e;
                    break;
                default:
                    // Internal error
                    sw = 0x6800u | (e & 0x7FFu);
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
        END_TRY
    }
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *)element);
}

void handle_SEPROXYHAL_TAG_FINGER_EVENT() {
    UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
}

void handle_SEPROXYHAL_TAG_BUTTON_PUSH_EVENT() {
    UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
}

void handle_SEPROXYHAL_TAG_STATUS_EVENT() {
    if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
        !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
          SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
        THROW(EXCEPTION_IO_RESET);
    }
}

void handle_default() {
    UX_DEFAULT_EVENT();
}

void handle_SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT() {
    UX_DISPLAYED_EVENT({});
}

void handle_SEPROXYHAL_TAG_TICKER_EVENT() {
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
    case SEPROXYHAL_TAG_FINGER_EVENT:
        handle_SEPROXYHAL_TAG_FINGER_EVENT();
        break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        handle_SEPROXYHAL_TAG_BUTTON_PUSH_EVENT();
        break;

    case SEPROXYHAL_TAG_STATUS_EVENT:
        handle_SEPROXYHAL_TAG_STATUS_EVENT();
    // no break is intentional
    default:
        handle_default();
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        handle_SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT();
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        handle_SEPROXYHAL_TAG_TICKER_EVENT();
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
    END_TRY_L(exit)
}

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        resetTransactionContext();
	
        UX_INIT()
        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

#ifdef TARGET_NANOX
                // grab the current plane mode setting
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif // TARGET_NANOX

                USB_power(0);
                USB_power(1);

                displayIdleMenu();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif // HAVE_BLE

                sample_main();
            }
                CATCH(EXCEPTION_IO_RESET) {
                    // reset IO and UX
                    continue;
                }
                CATCH_ALL {
                    break;
                }
            FINALLY {
            }
        }
        END_TRY
    }
    app_exit();

    return 0;
}
