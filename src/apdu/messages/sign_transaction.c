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
#include <os_io_usb.h>
#include "sign_transaction.h"
#include "constants.h"
#include "global.h"
#include "transaction.h"
#include "idle_menu.h"
#include "xrp_helpers.h"
#include "crypto_helpers.h"

static const uint8_t prefix_length = 4;
static const uint8_t suffix_length = 20;

static const uint8_t sign_prefix[] = {0x53, 0x54, 0x58, 0x00};
static const uint8_t sign_prefix_multi[] = {0x53, 0x4D, 0x54, 0x00};

parseContext_t parse_context;

void handle_packet_content(uint8_t p1,
                           uint8_t p2,
                           uint8_t *work_buffer,
                           uint8_t data_length,
                           volatile unsigned int *flags);

void sign_transaction() {
    uint8_t key_buffer[64];
    cx_ecfp_private_key_t private_key;
    uint32_t info, tx = 0;

    if (sign_state != PENDING_REVIEW) {
        reset_transaction_context();
        display_idle_menu();
        return;
    }

    // Abort if we accidentally end up here again after the transaction has already been signed
    if (parse_context.data == NULL) {
        display_idle_menu();
        return;
    }

    io_seproxyhal_io_heartbeat();

    cx_err_t error = CX_INTERNAL_ERROR;
    CX_CHECK(bip32_derive_init_privkey_256(tmp_ctx.transaction_context.curve,
                                           tmp_ctx.transaction_context.bip32_path,
                                           tmp_ctx.transaction_context.path_length,
                                           &private_key,
                                           NULL));

    io_seproxyhal_io_heartbeat();

    // Append public key to end of transaction if multi-signing
    if (parse_context.has_empty_pub_key) {
        cx_ecfp_public_key_t public_key;

        xrp_pubkey_t *public_key_data = (xrp_pubkey_t *) key_buffer;
        uint8_t *suffix_data = key_buffer + XRP_PUBKEY_SIZE;

        CX_CHECK(cx_ecfp_generate_pair_no_throw(tmp_ctx.transaction_context.curve,
                                                &public_key,
                                                &private_key,
                                                1));
        xrp_compress_public_key(&public_key, public_key_data);
        xrp_public_key_hash160(public_key_data, suffix_data);

        memmove(tmp_ctx.transaction_context.raw_tx + tmp_ctx.transaction_context.raw_tx_length,
                suffix_data,
                suffix_length);
        tmp_ctx.transaction_context.raw_tx_length += suffix_length;

        explicit_bzero(key_buffer, sizeof(key_buffer));
    }

    if (tmp_ctx.transaction_context.curve == CX_CURVE_256K1) {
        cx_hash_sha512(tmp_ctx.transaction_context.raw_tx,
                       tmp_ctx.transaction_context.raw_tx_length,
                       key_buffer,
                       64);
        PRINTF("Hash to sign:\n%.*H\n", 32, key_buffer);
        io_seproxyhal_io_heartbeat();

        tx = sizeof(G_io_apdu_buffer);
        CX_CHECK(cx_ecdsa_sign_no_throw(&private_key,
                                        CX_RND_RFC6979 | CX_LAST,
                                        CX_SHA256,
                                        key_buffer,
                                        32,
                                        G_io_apdu_buffer,
                                        &tx,
                                        &info));
    } else {
        size_t size;
        CX_CHECK(cx_eddsa_sign_no_throw(&private_key,
                                        CX_SHA512,
                                        tmp_ctx.transaction_context.raw_tx,
                                        tmp_ctx.transaction_context.raw_tx_length,
                                        G_io_apdu_buffer,
                                        sizeof(G_io_apdu_buffer)));
        CX_CHECK(cx_ecdomain_parameters_length(private_key.curve, &size));
        tx = size * 2;
    }

end:
    explicit_bzero(key_buffer, sizeof(key_buffer));
    explicit_bzero(&private_key, sizeof(private_key));

    // Always reset transaction context after a transaction has been signed
    reset_transaction_context();

    if (error != CX_OK) {
        THROW(error);
    }

    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;

    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

    if (called_from_swap) {
        return;
    }

    // Display back the original UX
    display_idle_menu();
}

void reject_transaction() {
    if (sign_state != PENDING_REVIEW) {
        reset_transaction_context();
        display_idle_menu();
        return;
    }

    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;

    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);

    if (called_from_swap) {
        return;
    }

    // Reset transaction context and display back the original UX
    reset_transaction_context();
    display_idle_menu();
}

bool is_first(uint8_t p1) {
    return (p1 & P1_MASK_ORDER) == 0;
}

bool has_more(uint8_t p1) {
    return (p1 & P1_MASK_MORE) != 0;
}

void handle_first_packet(uint8_t p1,
                         uint8_t p2,
                         uint8_t *work_buffer,
                         uint8_t data_length,
                         volatile unsigned int *flags) {
    if (!is_first(p1)) {
        THROW(0x6A80);
    }

    // Reset old transaction data that might still remain
    reset_transaction_context();
    parse_context.data = tmp_ctx.transaction_context.raw_tx + prefix_length;

    size_t path_length = work_buffer[0];
    uint32_t *path_parsed = tmp_ctx.transaction_context.bip32_path;

    work_buffer++;
    data_length--;
    if (!parse_bip32_path(work_buffer, path_length, path_parsed, MAX_BIP32_PATH)) {
        PRINTF("Invalid path\n");
        THROW(0x6a81);
    }

    tmp_ctx.transaction_context.path_length = path_length;
    work_buffer += sizeof(uint32_t) * tmp_ctx.transaction_context.path_length;
    data_length -= sizeof(uint32_t) * tmp_ctx.transaction_context.path_length;

    if (((p2 & P2_SECP256K1) == 0) && ((p2 & P2_ED25519) == 0)) {
        THROW(0x6B00);
    }
    if (((p2 & P2_SECP256K1) != 0) && ((p2 & P2_ED25519) != 0)) {
        THROW(0x6B00);
    }
    tmp_ctx.transaction_context.curve =
        (((p2 & P2_ED25519) != 0) ? CX_CURVE_Ed25519 : CX_CURVE_256K1);

    handle_packet_content(p1, p2, work_buffer, data_length, flags);
}

void handle_subsequent_packet(uint8_t p1,
                              uint8_t p2,
                              uint8_t *work_buffer,
                              uint8_t data_length,
                              volatile unsigned int *flags) {
    if (is_first(p1)) {
        THROW(0x6A80);
    }

    handle_packet_content(p1, p2, work_buffer, data_length, flags);
}

void handle_packet_content(uint8_t p1,
                           uint8_t p2,
                           uint8_t *work_buffer,
                           uint8_t data_length,
                           volatile unsigned int *flags) {
    UNUSED(p2);

    uint16_t total_length = prefix_length + parse_context.length + data_length;
    if (total_length > MAX_RAW_TX) {
        // Abort if the user is trying to sign a too large transaction
        THROW(0x6700);
    }

    // Append received data to stored transaction data
    memmove(parse_context.data + parse_context.length, work_buffer, data_length);
    parse_context.length += data_length;

    if (has_more(p1)) {
        // Reply to sender with status OK
        sign_state = WAITING_FOR_MORE;
        THROW(0x9000);
    } else {
        // No more data to receive, finish up and present transaction to user
        sign_state = PENDING_REVIEW;

        tmp_ctx.transaction_context.raw_tx_length = prefix_length + parse_context.length;

        // Try to parse the transaction. If the parsing fails an exception is thrown,
        // causing the processing to abort and the transaction context to be reset.
        int exception = parse_tx(&parse_context);
        if (exception) {
            THROW(exception);
        }

        // Set transaction prefix (space has been reserved earlier)
        if (parse_context.has_empty_pub_key) {
            if (tmp_ctx.transaction_context.raw_tx_length + suffix_length > MAX_RAW_TX) {
                // Abort if the added account ID suffix causes the transaction to be too large
                THROW(0x6700);
            }

            memmove(tmp_ctx.transaction_context.raw_tx, sign_prefix_multi, prefix_length);
        } else {
            memmove(tmp_ctx.transaction_context.raw_tx, sign_prefix, prefix_length);
        }

        review_transaction(&parse_context.result, sign_transaction, reject_transaction);

        *flags |= IO_ASYNCH_REPLY;
    }
}

void handle_sign(uint8_t p1,
                 uint8_t p2,
                 uint8_t *work_buffer,
                 uint8_t data_length,
                 volatile unsigned int *flags) {
    switch (sign_state) {
        case IDLE:
            handle_first_packet(p1, p2, work_buffer, data_length, flags);
            break;
        case WAITING_FOR_MORE:
            handle_subsequent_packet(p1, p2, work_buffer, data_length, flags);
            break;
        default:
            THROW(0x6A80);
    }
}
