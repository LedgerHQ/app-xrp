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
#include <status_words.h>
#include <string.h>

#include "address_ui.h"
#include "constants.h"
#include "global.h"
#include "idle_menu.h"
#include "io.h"
#include "os_io_usb.h"
#include "xrp_helpers.h"
#include "xrp_parse.h"
#include "xrp_pub_key.h"

static bool pubkey_confirmation_pending = false;
static publicKeyContext_t pending_pubkey_ctx;

int set_result_get_public_key(publicKeyContext_t* ctx) {
    uint8_t buf[1 + XRP_PUBKEY_SIZE + 1 + XRP_ADDRESS_SIZE + 32];
    uint32_t tx = 0;
    uint32_t address_length = strlen(ctx->address.buf);
    buf[tx++] = XRP_PUBKEY_SIZE;
    xrp_compress_public_key(&ctx->public_key, (xrp_pubkey_t*)(buf + tx));
    tx += XRP_PUBKEY_SIZE;
    buf[tx++] = address_length;
    memmove(buf + tx, ctx->address.buf, address_length);
    tx += address_length;
    if (ctx->get_chaincode) {
        memmove(buf + tx, ctx->chain_code, 32);
        tx += 32;
    }
    return io_send_response_pointer(buf, tx, 0x9000);
}

void on_address_confirmed() {
    pubkey_confirmation_pending = false;
    // Serialize directly from pending_pubkey_ctx rather than copying it into
    // the shared tmp_ctx union first: tmp_ctx also holds the in-progress
    // signing context, which this asynchronous callback must never touch.
    set_result_get_public_key(&pending_pubkey_ctx);
    explicit_bzero(&pending_pubkey_ctx, sizeof(pending_pubkey_ctx));
#ifndef HAVE_NBGL
    // Display back the original UX
    display_idle_menu();
#endif
}

void on_address_rejected() {
    pubkey_confirmation_pending = false;
    explicit_bzero(&pending_pubkey_ctx, sizeof(pending_pubkey_ctx));
    io_send_sw(0x6985);
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
        int result = set_result_get_public_key(&pending_pubkey_ctx);
        explicit_bzero(&pending_pubkey_ctx, sizeof(pending_pubkey_ctx));
        return result;
    } else {
        pubkey_confirmation_pending = true;
        if (!display_address_confirmation_ui(
                pending_pubkey_ctx.address.buf,
                strlen(pending_pubkey_ctx.address.buf), on_address_confirmed,
                on_address_rejected)) {
            // Nothing was displayed and neither callback will ever fire, so
            // don't leave the device waiting on a confirmation that will
            // never come.
            pubkey_confirmation_pending = false;
            explicit_bzero(&pending_pubkey_ctx, sizeof(pending_pubkey_ctx));
            return io_send_sw(SWO_INCORRECT_DATA);
        }

        return 0;
    }
}
