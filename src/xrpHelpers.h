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

unsigned short xrp_public_key_to_encoded_base58(
    unsigned char WIDE *in, unsigned short inlen, unsigned char *out,
    unsigned short outlen, unsigned short version,
    unsigned char alreadyHashed);

unsigned short xrp_decode_base58_address(unsigned char WIDE *in,
                                            unsigned short inlen,
                                            unsigned char *out,
                                            unsigned short outlen);

unsigned short xrp_compress_public_key(cx_ecfp_public_key_t *publicKey, uint8_t *out, uint32_t outlen);

unsigned short xrp_print_amount(uint64_t amount, uint8_t *out, uint32_t outlen);
