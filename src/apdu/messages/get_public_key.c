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
#include <string.h>

#include "os_io_usb.h"
#include "get_public_key.h"
#include "constants.h"
#include "global.h"
#include "xrp_helpers.h"
#include "xrp_pub_key.h"
#include "xrp_parse.h"
#include "address_ui.h"
#include "idle_menu.h"

uint32_t set_result_get_public_key() {
    uint32_t tx = 0;
    uint32_t address_length = strlen(tmp_ctx.public_key_context.address.buf);
    G_io_apdu_buffer[tx++] = XRP_PUBKEY_SIZE;
    xrp_pubkey_t *pubkey = (xrp_pubkey_t *) (G_io_apdu_buffer + tx);
    xrp_compress_public_key(&tmp_ctx.public_key_context.public_key, pubkey);
    tx += XRP_PUBKEY_SIZE;
    G_io_apdu_buffer[tx++] = address_length;
    memmove(G_io_apdu_buffer + tx, tmp_ctx.public_key_context.address.buf, address_length);
    tx += address_length;
    if (tmp_ctx.public_key_context.get_chaincode) {
        memmove(G_io_apdu_buffer + tx, tmp_ctx.public_key_context.chain_code, 32);
        tx += 32;
    }
    return tx;
}

void on_address_confirmed() {
    uint32_t tx = set_result_get_public_key();
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
#ifndef HAVE_NBGL
    // Display back the original UX
    display_idle_menu();
#endif
}

void on_address_rejected() {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
#ifndef HAVE_NBGL
    // Display back the original UX
    display_idle_menu();
#endif
}

void handle_get_public_key(uint8_t p1,
                           uint8_t p2,
                           uint8_t *data_buffer,
                           uint16_t data_length,
                           volatile unsigned int *flags,
                           volatile unsigned int *tx) {
    UNUSED(data_length);

    uint8_t bip32_path_length = *(data_buffer++);
    uint8_t p2_chain = p2 & 0x3Fu;
    cx_curve_t curve;

    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        THROW(0x6B00);
    }
    if ((p2_chain != P2_CHAINCODE) && (p2_chain != P2_NO_CHAINCODE)) {
        THROW(0x6B00);
    }
    if (((p2 & P2_SECP256K1) == 0) && ((p2 & P2_ED25519) == 0)) {
        THROW(0x6B00);
    }
    if (((p2 & P2_SECP256K1) != 0) && ((p2 & P2_ED25519) != 0)) {
        THROW(0x6B00);
    }

    curve = (((p2 & P2_ED25519) != 0) ? CX_CURVE_Ed25519 : CX_CURVE_256K1);
    tmp_ctx.public_key_context.get_chaincode = (p2_chain == P2_CHAINCODE);
    uint8_t *chain_code =
        tmp_ctx.public_key_context.get_chaincode ? tmp_ctx.public_key_context.chain_code : NULL;

    io_seproxyhal_io_heartbeat();
    int error;
    error = get_public_key(curve,
                           data_buffer,
                           bip32_path_length,
                           &tmp_ctx.public_key_context.public_key,
                           chain_code);
    if (error != 0) {
        THROW(error);
    }

    io_seproxyhal_io_heartbeat();
    get_address(&tmp_ctx.public_key_context.public_key, &tmp_ctx.public_key_context.address);

    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_public_key();
        THROW(0x9000);
    } else {
        display_address_confirmation_ui(tmp_ctx.public_key_context.address.buf,
                                        on_address_confirmed,
                                        on_address_rejected);

        *flags |= IO_ASYNCH_REPLY;
    }
}
