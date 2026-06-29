#pragma once

#include <stdint.h>

#include "constants.h"

#define BLIND_SIGN_REQUIRED 20

/**
 * Blind signing options.
 */
enum BlindSign {
    BlindSignDisabled = 0,
    BlindSignEnabled = 1,
};

/**
 * Global structure for NVM data storage.
 */
typedef struct internal_storage_t {
    uint8_t allow_blind_sign;
    uint8_t initialized;
} internal_storage_t;

extern const internal_storage_t N_storage_real;
#define N_storage (*(volatile internal_storage_t *) PIC(&N_storage_real))
