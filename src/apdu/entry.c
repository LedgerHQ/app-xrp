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

#include <os.h>
#include "constants.h"
#include "global.h"
#include "entry.h"
#include "messages/get_public_key.h"
#include "messages/sign_transaction.h"
#include "messages/get_app_configuration.h"

static unsigned char last_ins = 0;

void handle_apdu(volatile unsigned int *flags, volatile unsigned int *tx) {
    unsigned short sw = 0;

    BEGIN_TRY {
        TRY {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(0x6E00);
            }

            // Reset transaction context before starting to parse a new APDU message type.
            // This helps protect against "Instruction Change" attacks
            if (G_io_apdu_buffer[OFFSET_INS] != last_ins) {
                reset_transaction_context();
            }

            last_ins = G_io_apdu_buffer[OFFSET_INS];

            switch (G_io_apdu_buffer[OFFSET_INS]) {
                case INS_GET_PUBLIC_KEY:
                    handle_get_public_key(G_io_apdu_buffer[OFFSET_P1],
                                          G_io_apdu_buffer[OFFSET_P2],
                                          G_io_apdu_buffer + OFFSET_CDATA,
                                          G_io_apdu_buffer[OFFSET_LC],
                                          flags,
                                          tx);
                    break;

                case INS_SIGN:
                    handle_sign(G_io_apdu_buffer[OFFSET_P1],
                                G_io_apdu_buffer[OFFSET_P2],
                                G_io_apdu_buffer + OFFSET_CDATA,
                                G_io_apdu_buffer[OFFSET_LC],
                                flags);
                    break;

                case INS_GET_APP_CONFIGURATION:
                    handle_get_app_configuration(tx);
                    break;

                default:
                    THROW(0x6D00);
                    break;
            }
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
                    // Internal error, wipe the transaction context and report the exception
                    sw = 0x6800u | (e & 0x7FFu);
                    reset_transaction_context();
                    break;
            }
            // Unexpected exception => report
            G_io_apdu_buffer[*tx] = sw >> 8u;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY {
        }
    }
    END_TRY;
}
