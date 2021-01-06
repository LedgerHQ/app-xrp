#include <string.h>

#include "xrp_pub_key.h"
#include "xrp_helpers.h"

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

    cx_ecfp_private_key_t private_key;
    uint8_t private_key_data[33];
    int error = 0;

    BEGIN_TRY {
        TRY {
            os_perso_derive_node_bip32(curve,
                                       bip32_path_parsed,
                                       bip32_path_length,
                                       private_key_data,
                                       chain_code);
            cx_ecfp_init_private_key(curve, private_key_data, 32, &private_key);
            cx_ecfp_generate_pair(curve, pub_key, &private_key, 1);
        }
        CATCH_OTHER(e) {
            error = e;
        }
        FINALLY {
            explicit_bzero(private_key_data, sizeof(private_key_data));
            explicit_bzero(&private_key, sizeof(private_key));
        }
    }
    END_TRY;

    return error;
}
