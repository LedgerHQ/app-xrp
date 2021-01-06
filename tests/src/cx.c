#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <cmocka.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>

#include "cx.h"

int cx_sha256_init(cx_sha256_t *hash) {
    memset(hash, 0, sizeof(cx_sha256_t));
    hash->header.algo = CX_SHA256;

    return CX_SHA256;
}

int cx_ripemd160_init(cx_ripemd160_t *hash) {
    memset(hash, 0, sizeof(cx_ripemd160_t));
    hash->header.algo = CX_RIPEMD160;

    return CX_RIPEMD160;
}

int cx_hash(cx_hash_t *hash,
            int mode,
            const uint8_t *in,
            size_t len,
            uint8_t *out,
            size_t out_len) {
    assert_int_equal(mode, CX_LAST);

    if (hash->algo == CX_SHA256) {
        // uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, in, len);
        SHA256_Final(out, &sha256);
    } else if (hash->algo == CX_RIPEMD160) {
        RIPEMD160_CTX ripemd160;
        RIPEMD160_Init(&ripemd160);
        RIPEMD160_Update(&ripemd160, in, len);
        RIPEMD160_Final(out, &ripemd160);
    } else {
        assert_true(false);
    }

    return 0;
}
