#include <string.h>

#include "xrp_pub_key.h"
#include "xrp_helpers.h"
#include "crypto_helpers.h"

/* return 0 on success */
int get_public_key(cx_curve_t curve,
                   uint8_t *bip32_path,
                   size_t bip32_path_length,
                   cx_ecfp_public_key_t *pub_key,
                   uint8_t *chain_code) {
    uint32_t bip32_path_parsed[MAX_BIP32_PATH];

    if (!parse_bip32_path(bip32_path, bip32_path_length, bip32_path_parsed, MAX_BIP32_PATH)) {
        PRINTF("Invalid path\n");
        return 0x6a80;
    }

    pub_key->curve = curve;
    pub_key->W_len = 65;
    return bip32_derive_get_pubkey_256(curve, bip32_path_parsed, bip32_path_length, pub_key->W, chain_code, CX_SHA256);
}
