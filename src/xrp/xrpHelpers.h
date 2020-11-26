/*******************************************************************************
 *   XRP Wallet
 *   (c) 2017 Ledger
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

#include "os.h"
#include "cx.h"

void xrp_public_key_hash160(unsigned char WIDE *in, unsigned short inlen, unsigned char *out);

unsigned short xrp_public_key_to_encoded_base58(unsigned char WIDE *in,
                                                unsigned short inlen,
                                                char *out,
                                                unsigned short outlen,
                                                unsigned short version,
                                                unsigned char alreadyHashed);

void xrp_compress_public_key(cx_ecfp_public_key_t *publicKey, uint8_t *out, uint32_t outlen);

void get_publicKey(cx_curve_t curve,
                   uint8_t *bip32Path,
                   size_t bip32PathLength,
                   cx_ecfp_public_key_t *pubKey,
                   uint8_t *chainCode);

void get_address(cx_ecfp_public_key_t *pubkey, char *address, size_t maxAddressLength);

void xrp_print_amount(uint64_t amount, char *out, uint32_t outlen);
