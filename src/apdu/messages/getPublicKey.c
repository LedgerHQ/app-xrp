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
#include "getPublicKey.h"
#include "../constants.h"
#include "../global.h"
#include "../../xrp/xrpHelpers.h"
#include "../../xrp/parse/xrpParse.h"
#include "../../ui/address/addressUI.h"
#include "../../ui/main/idleMenu.h"

uint32_t set_result_get_publicKey() {
    uint32_t tx = 0;
    uint32_t addressLength = strlen(tmpCtx.publicKeyContext.address);
    G_io_apdu_buffer[tx++] = 33;
    xrp_compress_public_key(&tmpCtx.publicKeyContext.publicKey, G_io_apdu_buffer + tx, 33);
    tx += 33;
    G_io_apdu_buffer[tx++] = addressLength;
    os_memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.address, addressLength);
    tx += addressLength;
    if (tmpCtx.publicKeyContext.getChaincode) {
        os_memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.chainCode, 32);
        tx += 32;
    }
    return tx;
}

void onAddressConfirmed() {
    uint32_t tx = set_result_get_publicKey();
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    displayIdleMenu();
}

void onAddressRejected() {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    displayIdleMenu();
}

void handleGetPublicKey(uint8_t p1,
                        uint8_t p2,
                        uint8_t *dataBuffer,
                        uint16_t dataLength,
                        volatile unsigned int *flags,
                        volatile unsigned int *tx) {
    UNUSED(dataLength);
    uint8_t privateKeyData[33];
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint32_t i;
    uint8_t bip32PathLength = *(dataBuffer++);
    cx_ecfp_private_key_t privateKey;
    uint8_t p2Chain = p2 & 0x3Fu;
    cx_curve_t curve;
    uint8_t addressLength;

    if ((bip32PathLength < 0x01) || (bip32PathLength > MAX_BIP32_PATH)) {
        PRINTF("Invalid path\n");
        THROW(0x6a80);
    }
    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        THROW(0x6B00);
    }
    if ((p2Chain != P2_CHAINCODE) && (p2Chain != P2_NO_CHAINCODE)) {
        THROW(0x6B00);
    }
    if (((p2 & P2_SECP256K1) == 0) && ((p2 & P2_ED25519) == 0)) {
        THROW(0x6B00);
    }
    if (((p2 & P2_SECP256K1) != 0) && ((p2 & P2_ED25519) != 0)) {
        THROW(0x6B00);
    }
    curve = (((p2 & P2_ED25519) != 0) ? CX_CURVE_Ed25519 : CX_CURVE_256K1);

    for (i = 0; i < bip32PathLength; i++) {
        bip32Path[i] = (dataBuffer[0] << 24u) | (dataBuffer[1] << 16u) | (dataBuffer[2] << 8u) |
                       (dataBuffer[3]);
        dataBuffer += 4;
    }
    tmpCtx.publicKeyContext.getChaincode = (p2Chain == P2_CHAINCODE);
    io_seproxyhal_io_heartbeat();

    BEGIN_TRY {
        TRY {
            os_perso_derive_node_bip32(
                curve,
                bip32Path,
                bip32PathLength,
                privateKeyData,
                (tmpCtx.publicKeyContext.getChaincode ? tmpCtx.publicKeyContext.chainCode : NULL));
            cx_ecfp_init_private_key(curve, privateKeyData, 32, &privateKey);
            io_seproxyhal_io_heartbeat();
            cx_ecfp_generate_pair(curve, &tmpCtx.publicKeyContext.publicKey, &privateKey, 1);
            explicit_bzero(&privateKey, sizeof(privateKey));
            explicit_bzero(privateKeyData, sizeof(privateKeyData));
            io_seproxyhal_io_heartbeat();
            xrp_compress_public_key(&tmpCtx.publicKeyContext.publicKey, privateKeyData, 33);
            io_seproxyhal_io_heartbeat();
            addressLength =
                xrp_public_key_to_encoded_base58(privateKeyData,
                                                 33,
                                                 tmpCtx.publicKeyContext.address,
                                                 sizeof(tmpCtx.publicKeyContext.address),
                                                 0,
                                                 0);
            tmpCtx.publicKeyContext.address[addressLength] = '\0';
        }
        CATCH_OTHER(e) {
            THROW(e);
        }
        FINALLY {
            explicit_bzero(privateKeyData, sizeof(privateKeyData));
            explicit_bzero(&privateKey, sizeof(privateKey));
        }
    }
    END_TRY

    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_publicKey();
        THROW(0x9000);
    } else {
        displayAddressConfirmationUI(tmpCtx.publicKeyContext.address,
                                     onAddressConfirmed,
                                     onAddressRejected);

        *flags |= IO_ASYNCH_REPLY;
    }
}
