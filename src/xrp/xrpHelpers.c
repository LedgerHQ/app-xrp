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

#include "xrpHelpers.h"
#include "xrpBase58.h"
#include "parse/numberHelpers.h"
#include <stdbool.h>
#include <string.h>

void xrp_public_key_hash160(unsigned char WIDE *in, unsigned short inlen,
                               unsigned char *out) {
    union {
        cx_sha256_t shasha;
        cx_ripemd160_t riprip;
    } u;
    unsigned char buffer[32];

    cx_sha256_init(&u.shasha);
    cx_hash(&u.shasha.header, CX_LAST, in, inlen, buffer, 32);
    cx_ripemd160_init(&u.riprip);
    cx_hash(&u.riprip.header, CX_LAST, buffer, 32, out, 20);
}

unsigned short xrp_public_key_to_encoded_base58(
    unsigned char WIDE *in, unsigned short inlen, char *out,
    unsigned short outlen, unsigned short version,
    unsigned char alreadyHashed) {
    unsigned char tmpBuffer[26];
    unsigned char checksumBuffer[32];
    cx_sha256_t hash;
    unsigned char versionSize = (version > 255 ? 2 : 1);

    if (version > 255) {
        tmpBuffer[0] = (version >> 8u);
        tmpBuffer[1] = version;
    } else {
        tmpBuffer[0] = version;
    }

    if (!alreadyHashed) {
        xrp_public_key_hash160(in, inlen, tmpBuffer + versionSize);
    } else {
        os_memmove(tmpBuffer + versionSize, in, 20);
    }

    cx_sha256_init(&hash);
    cx_hash(&hash.header, CX_LAST, tmpBuffer, 20 + versionSize, checksumBuffer, 32);
    cx_sha256_init(&hash);
    cx_hash(&hash.header, CX_LAST, checksumBuffer, 32, checksumBuffer, 32);

    os_memmove(tmpBuffer + 20 + versionSize, checksumBuffer, 4);
    return xrp_encode_base58(tmpBuffer, 24 + versionSize, out, outlen);
}

void xrp_compress_public_key(cx_ecfp_public_key_t *publicKey, uint8_t *out, uint32_t outlen) {
    if (outlen < 33) {
        THROW(EXCEPTION_OVERFLOW);
    }
    if (publicKey->curve == CX_CURVE_256K1) {
        out[0] = ((publicKey->W[64] & 1u) ? 0x03 : 0x02);
        os_memmove(out + 1, publicKey->W + 1, 32);
    }
    else
    if (publicKey->curve == CX_CURVE_Ed25519) {
        uint8_t i;
        out[0] = 0xED;
        for (i=0; i<32; i++) {
            out[i + 1] = publicKey->W[64 - i];
        }
        if ((publicKey->W[32] & 1u) != 0) {
            out[32] |= 0x80u;
        }
    }
    else {
        THROW(EXCEPTION);
    }
}

bool adjustDecimals(const char *src, uint32_t srcLength, char *target,
                    uint32_t targetLength, uint8_t decimals) {
    uint32_t startOffset;
    uint32_t lastZeroOffset = 0;
    uint32_t offset = 0;

    if ((srcLength == 1) && (*src == '0')) {
        if (targetLength < 2) {
            return false;
        }
        target[offset++] = '0';
        target[offset++] = '\0';
        return true;
    }
    if (srcLength <= decimals) {
        uint32_t delta = decimals - srcLength;
        if (targetLength < srcLength + 1 + 2 + delta) {
            return false;
        }
        target[offset++] = '0';
        target[offset++] = '.';
        for (uint32_t i = 0; i < delta; i++) {
            target[offset++] = '0';
        }
        startOffset = offset;
        for (uint32_t i = 0; i < srcLength; i++) {
            target[offset++] = src[i];
        }
        target[offset] = '\0';
    } else {
        uint32_t sourceOffset = 0;
        uint32_t delta = srcLength - decimals;
        if (targetLength < srcLength + 1 + 1) {
            return false;
        }
        while (offset < delta) {
            target[offset++] = src[sourceOffset++];
        }
        if (decimals != 0) {
            target[offset++] = '.';
        }
        startOffset = offset;
        while (sourceOffset < srcLength) {
            target[offset++] = src[sourceOffset++];
        }
        target[offset] = '\0';
    }
    for (uint32_t i = startOffset; i < offset; i++) {
        if (target[i] == '0') {
            if (lastZeroOffset == 0) {
                lastZeroOffset = i;
            }
        } else {
            lastZeroOffset = 0;
        }
    }
    if (lastZeroOffset != 0) {
        target[lastZeroOffset] = '\0';
        if (target[lastZeroOffset - 1] == '.') {
            target[lastZeroOffset - 1] = '\0';
        }
    }
    return true;
}

void xrp_print_amount(uint64_t amount, char *out, uint32_t outlen) {
    char tmp[20];    
    char tmp2[25];
    uint32_t numDigits = 0, i;
    uint64_t base = 1;
    while (base <= amount) {
        base *= 10;
        numDigits++;
    }
    if (numDigits > sizeof(tmp) - 1) {
        THROW(EXCEPTION);
    }
    base /= 10;
    for (i=0; i<numDigits; i++) {
        tmp[i] = intToNumberChar((amount / base) % 10);
        base /= 10;
    }
    tmp[i] = '\0';
    strcpy(tmp2, "XRP ");
    adjustDecimals(tmp, i, tmp2 + 4, 25, 6);
    if (strlen(tmp2) < outlen - 1) {
        strcpy(out, tmp2);        
    }    
    else {
        out[0] = '\0';
    }
}

