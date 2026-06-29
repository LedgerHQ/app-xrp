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

#include "entry.h"

#include <os.h>

#include "constants.h"
#include "get_app_configuration.h"
#include "get_public_key.h"
#include "global.h"
#include "sign_transaction.h"
#include "io.h"

static unsigned char last_ins = 0;

int handle_apdu(void) {
    if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
        return io_send_sw(0x6E00);
    }

    if (G_io_apdu_buffer[OFFSET_INS] != last_ins) {
        reset_transaction_context();
    }

    last_ins = G_io_apdu_buffer[OFFSET_INS];

    switch (G_io_apdu_buffer[OFFSET_INS]) {
        case INS_GET_PUBLIC_KEY:
            return handle_get_public_key(G_io_apdu_buffer[OFFSET_P1],
                                         G_io_apdu_buffer[OFFSET_P2],
                                         G_io_apdu_buffer + OFFSET_CDATA,
                                         G_io_apdu_buffer[OFFSET_LC]);
        case INS_SIGN:
            return handle_sign(G_io_apdu_buffer[OFFSET_P1],
                               G_io_apdu_buffer[OFFSET_P2],
                               G_io_apdu_buffer + OFFSET_CDATA,
                               G_io_apdu_buffer[OFFSET_LC]);
        case INS_GET_APP_CONFIGURATION:
            return handle_get_app_configuration();
        default:
            return io_send_sw(0x6D00);
    }
}
