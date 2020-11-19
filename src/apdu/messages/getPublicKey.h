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

#ifndef LEDGER_APP_XRP_GETPUBLICKEY_H
#define LEDGER_APP_XRP_GETPUBLICKEY_H

#include <stdint.h>

void handleGetPublicKey(uint8_t p1,
                        uint8_t p2,
                        uint8_t *dataBuffer,
                        uint16_t dataLength,
                        volatile unsigned int *flags,
                        volatile unsigned int *tx);

void getPublicKey(cx_curve_t curve,
                  uint8_t *bip32Path,
                  size_t bip32PathLength,
                  cx_ecfp_public_key_t *pubKey,
                  uint8_t *chainCode);

void getAddress(cx_ecfp_public_key_t *pubkey, char *address, size_t maxAddressLength);

#endif  // LEDGER_APP_XRP_GETPUBLICKEY_H
