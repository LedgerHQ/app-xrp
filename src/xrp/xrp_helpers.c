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

#include <stdbool.h>
#include <string.h>

#include "xrp_helpers.h"
#include "os.h"
#include "number_helpers.h"
#include "limitations.h"

typedef struct {
    uint8_t buf[MAX_ENC_INPUT_SIZE];
    uint8_t length;
} base58_buf_t;

static const char bas_e58_alphabet[] = {'r', 'p', 's', 'h', 'n', 'a', 'f', '3', '9', 'w', 'B', 'U',
                                        'D', 'N', 'E', 'G', 'H', 'J', 'K', 'L', 'M', '4', 'P', 'Q',
                                        'R', 'S', 'T', '7', 'V', 'W', 'X', 'Y', 'Z', '2', 'b', 'c',
                                        'd', 'e', 'C', 'g', '6', '5', 'j', 'k', 'm', '8', 'o', 'F',
                                        'q', 'i', '1', 't', 'u', 'v', 'A', 'x', 'y', 'z'};

static size_t xrp_encode_base58_address(const base58_buf_t *in, xrp_address_t *out) {
    unsigned char buffer[MAX_ENC_INPUT_SIZE * 138 / 100 + 1] = {0};
    size_t i = 0, j;
    size_t start_at, stop_at;
    size_t zero_count = 0;
    size_t output_size;
    size_t outlen = sizeof(out->buf);

    while ((zero_count < in->length) && (in->buf[zero_count] == 0)) {
        ++zero_count;
    }

    output_size = (in->length - zero_count) * 138 / 100 + 1;
    stop_at = output_size - 1;
    for (start_at = zero_count; start_at < in->length; start_at++) {
        int carry = in->buf[start_at];
        for (j = output_size - 1; (int) j >= 0; j--) {
            carry += 256 * buffer[j];
            buffer[j] = carry % 58;
            carry /= 58;

            if (j <= stop_at - 1 && carry == 0) {
                break;
            }
        }
        stop_at = j;
    }

    j = 0;
    while (j < output_size && buffer[j] == 0) {
        j += 1;
    }

    memset(out, bas_e58_alphabet[0], zero_count);

    i = zero_count;
    while (j < output_size) {
        out->buf[i++] = bas_e58_alphabet[buffer[j++]];
    }
    outlen = i;

    return outlen;
}

static void xrp_hash_ripemd160(cx_ripemd160_t *hash,
                               const uint8_t *in,
                               size_t in_len,
                               uint8_t *out,
                               size_t out_len) {
    cx_ripemd160_init_no_throw(hash);
    cx_ripemd160_update(hash, in, in_len);
    cx_ripemd160_final(hash, out);
}

void xrp_public_key_hash160(xrp_pubkey_t *pubkey, uint8_t *out) {
    union {
        cx_sha256_t shasha;
        cx_ripemd160_t riprip;
    } u;
    uint8_t buffer[32];

    cx_sha256_init(&u.shasha);
    cx_hash(&u.shasha.header, CX_LAST, pubkey->buf, sizeof(pubkey->buf), buffer, 32);
    xrp_hash_ripemd160(&u.riprip, buffer, 32, out, 20);
}

size_t xrp_public_key_to_encoded_base58(xrp_pubkey_t *pubkey,
                                        xrp_account_t *account,
                                        xrp_address_t *out,
                                        uint16_t version) {
    base58_buf_t tmp;
    unsigned char checksum_buffer[32];
    cx_sha256_t hash;
    unsigned char version_size = (version > 255 ? 2 : 1);

    if (version > 255) {
        tmp.buf[0] = (version >> 8u);
        tmp.buf[1] = version;
    } else {
        tmp.buf[0] = version;
    }

    if (pubkey != NULL) {
        xrp_public_key_hash160(pubkey, tmp.buf + version_size);
    }
    if (account != NULL) {
        memmove(tmp.buf + version_size, account->buf, sizeof(account->buf));
    }

    cx_sha256_init(&hash);
    cx_hash(&hash.header, CX_LAST, tmp.buf, 20 + version_size, checksum_buffer, 32);
    cx_sha256_init(&hash);
    cx_hash(&hash.header, CX_LAST, checksum_buffer, 32, checksum_buffer, 32);

    memmove(tmp.buf + 20 + version_size, checksum_buffer, 4);
    tmp.length = 24 + version_size;

    return xrp_encode_base58_address(&tmp, out);
}

void xrp_compress_public_key(cx_ecfp_public_key_t *public_key, xrp_pubkey_t *out) {
    if (public_key->curve == CX_CURVE_256K1) {
        out->buf[0] = ((public_key->W[64] & 1u) ? 0x03 : 0x02);
        memmove(out->buf + 1, public_key->W + 1, 32);
    } else {
        // publicKey->curve == CX_CURVE_Ed25519
        uint8_t i;
        out->buf[0] = 0xED;
        for (i = 0; i < 32; i++) {
            out->buf[i + 1] = public_key->W[64 - i];
        }
        if ((public_key->W[32] & 1u) != 0) {
            out->buf[32] |= 0x80u;
        }
    }
}

bool parse_bip32_path(uint8_t *path,
                      size_t path_length,
                      uint32_t *path_parsed,
                      size_t path_parsed_length) {
    if ((path_length < 0x01) || (path_length > path_parsed_length)) {
        return false;
    }

    for (size_t i = 0; i < path_length; i++) {
        path_parsed[i] = (path[0] << 24u) | (path[1] << 16u) | (path[2] << 8u) | (path[3]);
        path += 4;
    }

    return true;
}

void get_address(cx_ecfp_public_key_t *pubkey, xrp_address_t *address) {
    /* sizeof(xrp_pubkey_t) < sizeof(xrp_address_t) */
    xrp_pubkey_t *p = (xrp_pubkey_t *) address;
    xrp_compress_public_key(pubkey, p);

    uint8_t addr_len = xrp_public_key_to_encoded_base58(p, NULL, address, 0);
    address->buf[addr_len] = '\x00';
}

bool adjust_decimals(const char *src,
                     uint32_t src_length,
                     char *target,
                     uint32_t target_length,
                     uint8_t decimals) {
    uint32_t start_offset;
    uint32_t last_zero_offset = 0;
    uint32_t offset = 0;

    if ((src_length == 1) && (*src == '0')) {
        if (target_length < 2) {
            return false;
        }
        target[offset++] = '0';
        target[offset++] = '\0';
        return true;
    }
    if (src_length <= decimals) {
        uint32_t delta = decimals - src_length;
        if (target_length < src_length + 1 + 2 + delta) {
            return false;
        }
        target[offset++] = '0';
        target[offset++] = '.';
        for (uint32_t i = 0; i < delta; i++) {
            target[offset++] = '0';
        }
        start_offset = offset;
        for (uint32_t i = 0; i < src_length; i++) {
            target[offset++] = src[i];
        }
        target[offset] = '\0';
    } else {
        uint32_t source_offset = 0;
        uint32_t delta = src_length - decimals;
        if (target_length < src_length + 1 + 1) {
            return false;
        }
        while (offset < delta) {
            target[offset++] = src[source_offset++];
        }
        if (decimals != 0) {
            target[offset++] = '.';
        }
        start_offset = offset;
        while (source_offset < src_length) {
            target[offset++] = src[source_offset++];
        }
        target[offset] = '\0';
    }
    for (uint32_t i = start_offset; i < offset; i++) {
        if (target[i] == '0') {
            if (last_zero_offset == 0) {
                last_zero_offset = i;
            }
        } else {
            last_zero_offset = 0;
        }
    }
    if (last_zero_offset != 0) {
        target[last_zero_offset] = '\0';
        if (target[last_zero_offset - 1] == '.') {
            target[last_zero_offset - 1] = '\0';
        }
    }
    return true;
}

#define CURRENCY      "XRP "
#define CURRENCY_SIZE (sizeof(CURRENCY) - 1)

/* return -1 on error, 0 otherwise */
int xrp_print_amount(uint64_t amount, char *out, size_t outlen) {
    char tmp[20];
    uint32_t num_digits = 0, i;
    uint64_t base;

    for (base = 1; base <= amount; base *= 10) {
        num_digits++;
        if (num_digits > sizeof(tmp) - 1) {
            return -1;
        }
    }

    base /= 10;
    for (i = 0; i < num_digits; i++) {
        tmp[i] = int_to_number_char((amount / base) % 10);
        base /= 10;
    }
    tmp[i] = '\0';

    char tmp2[25];
    strncpy(tmp2, CURRENCY, sizeof(tmp2));
    if (!adjust_decimals(tmp, i, tmp2 + CURRENCY_SIZE, sizeof(tmp2) - CURRENCY_SIZE, 6)) {
        return -1;
    }

    if (strlen(tmp2) >= outlen - 1) {
        out[0] = '\0';
        return -1;
    }
    strncpy(out, tmp2, outlen);

    return 0;
}
