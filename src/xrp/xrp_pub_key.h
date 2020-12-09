#pragma once

#include "os.h"

int get_public_key(cx_curve_t curve,
                   uint8_t *bip32_path,
                   size_t bip32_path_length,
                   cx_ecfp_public_key_t *pub_key,
                   uint8_t *chain_code);
