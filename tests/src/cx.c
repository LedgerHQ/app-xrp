#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <cmocka.h>
#include <openssl/sha.h>

#include "cx.h"

int cx_sha256_init(cx_sha256_t *hash) {
    memset(hash, 0, sizeof(cx_sha256_t));
    hash->header.algo = CX_SHA256;

    return CX_SHA256;
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
    } else {
        assert_true(false);
    }

    return 0;
}

int cx_ripemd160_init(cx_ripemd160_t *hash) {
  RIPEMD160_Init(&hash->ctx);
}

void cx_ripemd160_update(cx_ripemd160_t *hash,
                         const uint8_t *in,
                         size_t len) {
  RIPEMD160_Update(&hash->ctx, in, len);
}

void cx_ripemd160_final(cx_ripemd160_t *hash,
                        uint8_t *out) {
  RIPEMD160_Final(out, &hash->ctx);
}
