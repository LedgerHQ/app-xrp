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

#include "get_public_key.h"

#include <os.h>
#include <string.h>

#include "address_ui.h"
#include "constants.h"
#include "global.h"
#include "idle_menu.h"
#include "os_io_usb.h"
#include "xrp_helpers.h"
#include "xrp_parse.h"
#include "xrp_pub_key.h"
#include "io.h"

static bool pubkey_confirmation_pending = false;
static publicKeyContext_t pending_pubkey_ctx;

int set_result_get_public_key(void) {
    uint8_t buf[1 + XRP_PUBKEY_SIZE + 1 + XRP_ADDRESS_SIZE + 32];
    uint32_t tx = 0;
    uint32_t address_length = strlen(tmp_ctx.public_key_context.address.buf);
    buf[tx++] = XRP_PUBKEY_SIZE;
    xrp_compress_public_key(&tmp_ctx.public_key_context.public_key,
                            (xrp_pubkey_t*)(buf + tx));
    tx += XRP_PUBKEY_SIZE;
    buf[tx++] = address_length;
    memmove(buf + tx, tmp_ctx.public_key_context.address.buf, address_length);
    tx += address_length;
    if (tmp_ctx.public_key_context.get_chaincode) {
        memmove(buf + tx, tmp_ctx.public_key_context.chain_code, 32);
        tx += 32;
    }
    return io_send_response_pointer(buf, tx, 0x9000);
}

void on_address_confirmed() {
    pubkey_confirmation_pending = false;
    memcpy(&tmp_ctx.public_key_context, &pending_pubkey_ctx,
           sizeof(publicKeyContext_t));
    set_result_get_public_key();
#ifndef HAVE_NBGL
    // Display back the original UX
    display_idle_menu();
#endif
}

void on_address_rejected() {
    pubkey_confirmation_pending = false;
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
#ifndef HAVE_NBGL
    // Display back the original UX
    display_idle_menu();
#endif
}

int handle_get_public_key(uint8_t p1, uint8_t p2, uint8_t* data_buffer,
                          uint16_t data_length) {
    if (data_length < 1) {
        return io_send_sw(0x6a80);
    }

    if (pubkey_confirmation_pending) {
        return io_send_sw(0x6985);
    }

    uint8_t bip32_path_length = *(data_buffer++);
    data_length--;
    uint8_t p2_chain = p2 & 0x3Fu;
    cx_curve_t curve;

    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        return io_send_sw(0x6B00);
    }
    if ((p2_chain != P2_CHAINCODE) && (p2_chain != P2_NO_CHAINCODE)) {
        return io_send_sw(0x6B00);
    }
    if (((p2 & P2_SECP256K1) == 0) && ((p2 & P2_ED25519) == 0)) {
        return io_send_sw(0x6B00);
    }
    if (((p2 & P2_SECP256K1) != 0) && ((p2 & P2_ED25519) != 0)) {
        return io_send_sw(0x6B00);
    }

    curve = (((p2 & P2_ED25519) != 0) ? CX_CURVE_Ed25519 : CX_CURVE_256K1);
    memset(&pending_pubkey_ctx, 0, sizeof(pending_pubkey_ctx));
    pending_pubkey_ctx.get_chaincode = (p2_chain == P2_CHAINCODE);
    uint8_t* chain_code =
        pending_pubkey_ctx.get_chaincode ? pending_pubkey_ctx.chain_code : NULL;

    io_seproxyhal_io_heartbeat();
    int error;
    error = get_public_key(curve, data_buffer, bip32_path_length, data_length,
                           &pending_pubkey_ctx.public_key, chain_code);
    if (error != 0) {
        return io_send_sw(error);
    }

    io_seproxyhal_io_heartbeat();
    get_address(&pending_pubkey_ctx.public_key, &pending_pubkey_ctx.address);

    if (p1 == P1_NON_CONFIRM) {
        memcpy(&tmp_ctx.public_key_context, &pending_pubkey_ctx,
               sizeof(publicKeyContext_t));
        return set_result_get_public_key();
    } else {
        pubkey_confirmation_pending = true;
        display_address_confirmation_ui(pending_pubkey_ctx.address.buf,
                                        on_address_confirmed,
                                        on_address_rejected);

        return 0;
    }
}
