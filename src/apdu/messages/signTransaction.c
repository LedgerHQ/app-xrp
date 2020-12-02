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
#include "signTransaction.h"
#include "../constants.h"
#include "../global.h"
#include "../../transaction/transaction.h"
#include "../../ui/main/idleMenu.h"
#include "../../xrp/xrpHelpers.h"

static const uint8_t prefixLength = 4;
static const uint8_t pubKeyLength = 33;
static const uint8_t suffixLength = 20;

static const uint8_t SIGN_PREFIX[] = {0x53, 0x54, 0x58, 0x00};
static const uint8_t SIGN_PREFIX_MULTI[] = {0x53, 0x4D, 0x54, 0x00};

parseContext_t parseContext;

void handlePacketContent(uint8_t p1,
                         uint8_t p2,
                         uint8_t *workBuffer,
                         uint8_t dataLength,
                         volatile unsigned int *flags);

void signTransaction() {
    uint8_t privateKeyData[64];
    cx_ecfp_private_key_t privateKey;
    uint32_t tx = 0;

    if (signState != PENDING_REVIEW) {
        resetTransactionContext();
        displayIdleMenu();
        return;
    }

    // Abort if we accidentally end up here again after the transaction has already been signed
    if (parseContext.data == NULL) {
        displayIdleMenu();
        return;
    }

    int error = 0;
    BEGIN_TRY {
        TRY {
            io_seproxyhal_io_heartbeat();
            os_perso_derive_node_bip32(tmpCtx.transactionContext.curve,
                                       tmpCtx.transactionContext.bip32Path,
                                       tmpCtx.transactionContext.pathLength,
                                       privateKeyData,
                                       NULL);
            cx_ecfp_init_private_key(tmpCtx.transactionContext.curve,
                                     privateKeyData,
                                     32,
                                     &privateKey);
            explicit_bzero(privateKeyData, sizeof(privateKeyData));
            io_seproxyhal_io_heartbeat();

            // Append public key to end of transaction if multi-signing
            if (parseContext.hasEmptyPubKey) {
                cx_ecfp_public_key_t publicKey;

                // Re-use old buffer to save RAM
                uint8_t *publicKeyData = privateKeyData;
                uint8_t *suffixData = privateKeyData + pubKeyLength;

                cx_ecfp_generate_pair(tmpCtx.transactionContext.curve, &publicKey, &privateKey, 1);
                xrp_compress_public_key(&publicKey, publicKeyData, pubKeyLength);
                xrp_public_key_hash160(publicKeyData, pubKeyLength, suffixData);

                os_memmove(tmpCtx.transactionContext.rawTx + tmpCtx.transactionContext.rawTxLength,
                           suffixData,
                           suffixLength);
                tmpCtx.transactionContext.rawTxLength += suffixLength;

                explicit_bzero(privateKeyData, sizeof(privateKeyData));
            }

            if (tmpCtx.transactionContext.curve == CX_CURVE_256K1) {
                cx_hash_sha512(tmpCtx.transactionContext.rawTx,
                               tmpCtx.transactionContext.rawTxLength,
                               privateKeyData,
                               64);
                PRINTF("Hash to sign:\n%.*H\n", 32, privateKeyData);
                io_seproxyhal_io_heartbeat();
                tx = (uint32_t) cx_ecdsa_sign(&privateKey,
                                              CX_RND_RFC6979 | CX_LAST,
                                              CX_SHA256,
                                              privateKeyData,
                                              32,
                                              G_io_apdu_buffer,
                                              sizeof(G_io_apdu_buffer),
                                              NULL);
                G_io_apdu_buffer[0] = 0x30;
            } else {
                tx = (uint32_t) cx_eddsa_sign(&privateKey,
                                              CX_LAST,
                                              CX_SHA512,
                                              tmpCtx.transactionContext.rawTx,
                                              tmpCtx.transactionContext.rawTxLength,
                                              NULL,
                                              0,
                                              G_io_apdu_buffer,
                                              sizeof(G_io_apdu_buffer),
                                              NULL);
            }
        }
        CATCH_OTHER(e) {
            error = e;
        }
        FINALLY {
            explicit_bzero(privateKeyData, sizeof(privateKeyData));
            explicit_bzero(&privateKey, sizeof(privateKey));

            // Always reset transaction context after a transaction has been signed
            resetTransactionContext();
        }
    }
    END_TRY;

    if (error) {
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
    displayIdleMenu();
}

void rejectTransaction() {
    if (signState != PENDING_REVIEW) {
        resetTransactionContext();
        displayIdleMenu();
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
    resetTransactionContext();
    displayIdleMenu();
}

bool isFirst(uint8_t p1) {
    return (p1 & P1_MASK_ORDER) == 0;
}

bool hasMore(uint8_t p1) {
    return (p1 & P1_MASK_MORE) != 0;
}

void handleFirstPacket(uint8_t p1,
                       uint8_t p2,
                       uint8_t *workBuffer,
                       uint8_t dataLength,
                       volatile unsigned int *flags) {
    uint32_t i;

    if (!isFirst(p1)) {
        THROW(0x6A80);
    }

    // Reset old transaction data that might still remain
    resetTransactionContext();
    parseContext.data = tmpCtx.transactionContext.rawTx + prefixLength;

    size_t pathLength = workBuffer[0];
    uint32_t *pathParsed = tmpCtx.transactionContext.bip32Path;

    workBuffer++;
    dataLength--;
    if (!parse_bip32_path(workBuffer, pathLength, pathParsed, MAX_BIP32_PATH)) {
        PRINTF("Invalid path\n");
        THROW(0x6a81);
    }

    tmpCtx.transactionContext.pathLength = pathLength;
    workBuffer += sizeof(uint32_t) * tmpCtx.transactionContext.pathLength;
    dataLength -= sizeof(uint32_t) * tmpCtx.transactionContext.pathLength;

    if (((p2 & P2_SECP256K1) == 0) && ((p2 & P2_ED25519) == 0)) {
        THROW(0x6B00);
    }
    if (((p2 & P2_SECP256K1) != 0) && ((p2 & P2_ED25519) != 0)) {
        THROW(0x6B00);
    }
    tmpCtx.transactionContext.curve =
        (((p2 & P2_ED25519) != 0) ? CX_CURVE_Ed25519 : CX_CURVE_256K1);

    handlePacketContent(p1, p2, workBuffer, dataLength, flags);
}

void handleSubsequentPacket(uint8_t p1,
                            uint8_t p2,
                            uint8_t *workBuffer,
                            uint8_t dataLength,
                            volatile unsigned int *flags) {
    if (isFirst(p1)) {
        THROW(0x6A80);
    }

    handlePacketContent(p1, p2, workBuffer, dataLength, flags);
}

void handlePacketContent(uint8_t p1,
                         uint8_t p2,
                         uint8_t *workBuffer,
                         uint8_t dataLength,
                         volatile unsigned int *flags) {
    uint16_t totalLength = prefixLength + parseContext.length + dataLength;
    if (totalLength > MAX_RAW_TX) {
        // Abort if the user is trying to sign a too large transaction
        THROW(0x6700);
    }

    // Append received data to stored transaction data
    os_memmove(parseContext.data + parseContext.length, workBuffer, dataLength);
    parseContext.length += dataLength;

    if (hasMore(p1)) {
        // Reply to sender with status OK
        signState = WAITING_FOR_MORE;
        THROW(0x9000);
    } else {
        // No more data to receive, finish up and present transaction to user
        signState = PENDING_REVIEW;

        tmpCtx.transactionContext.rawTxLength = prefixLength + parseContext.length;

        // Try to parse the transaction. If the parsing fails an exception is thrown,
        // causing the processing to abort and the transaction context to be reset.
        parseTx(&parseContext);

        // Set transaction prefix (space has been reserved earlier)
        if (parseContext.hasEmptyPubKey) {
            if (tmpCtx.transactionContext.rawTxLength + suffixLength > MAX_RAW_TX) {
                // Abort if the added account ID suffix causes the transaction to be too large
                THROW(0x6700);
            }

            os_memmove(tmpCtx.transactionContext.rawTx, SIGN_PREFIX_MULTI, prefixLength);
        } else {
            os_memmove(tmpCtx.transactionContext.rawTx, SIGN_PREFIX, prefixLength);
        }

        reviewTransaction(&parseContext.result, signTransaction, rejectTransaction);

        *flags |= IO_ASYNCH_REPLY;
    }
}

void handleSign(uint8_t p1,
                uint8_t p2,
                uint8_t *workBuffer,
                uint8_t dataLength,
                volatile unsigned int *flags) {
    switch (signState) {
        case IDLE:
            handleFirstPacket(p1, p2, workBuffer, dataLength, flags);
            break;
        case WAITING_FOR_MORE:
            handleSubsequentPacket(p1, p2, workBuffer, dataLength, flags);
            break;
        default:
            THROW(0x6A80);
    }
}
