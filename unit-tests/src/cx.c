#include <openssl/evp.h>
#include <openssl/opensslv.h>
#include <openssl/ripemd.h>
#include <openssl/sha.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/provider.h>
#endif

// When built under MemorySanitizer, OpenSSL is usually uninstrumented so MSan
// can't see that EVP_DigestFinal initializes its output buffer. Unpoison it
// explicitly to avoid false positives downstream.
#if defined(__has_feature)
#if __has_feature(memory_sanitizer)
#include <sanitizer/msan_interface.h>
#define MSAN_UNPOISON(p, n) __msan_unpoison((p), (n))
#endif
#endif
#ifndef MSAN_UNPOISON
#define MSAN_UNPOISON(p, n) ((void)0)
#endif

#include "cx.h"

int cx_sha256_init(cx_sha256_t* hash) {
    memset(hash, 0, sizeof(cx_sha256_t));
    hash->header.algo = CX_SHA256;

    return CX_SHA256;
}

int cx_ripemd160_init(cx_ripemd160_t* hash) {
    memset(hash, 0, sizeof(cx_ripemd160_t));
    hash->header.algo = CX_RIPEMD160;

    return CX_RIPEMD160;
}

int cx_hash_no_throw(cx_hash_t* hash, int mode, const uint8_t* in, size_t len,
                     uint8_t* out, size_t out_len) {
    if (hash == NULL) {
        return CX_INVALID_PARAMETER;
    }

    // This mock only ever finalizes in one shot; a caller relying on
    // incremental hashing (mode without CX_LAST) would silently get a
    // digest that never saw the earlier chunks.
    if (mode != CX_LAST) {
        return CX_INVALID_PARAMETER;
    }

    const EVP_MD* md;
    if (hash->algo == CX_SHA256) {
        md = EVP_sha256();
    } else if (hash->algo == CX_RIPEMD160) {
        md = EVP_ripemd160();
    } else {
        return CX_INVALID_PARAMETER;
    }

    // Validated before any OpenSSL call: EVP_DigestFinal() always writes the
    // full digest size to `out`, regardless of out_len, so an undersized
    // buffer must be rejected up front rather than after the write.
    int expected_size = EVP_MD_size(md);
    if (out == NULL || expected_size <= 0 || out_len < (size_t)expected_size) {
        return CX_INVALID_PARAMETER_SIZE;
    }

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    OSSL_PROVIDER* prov = NULL;
    if (hash->algo == CX_RIPEMD160) {
        prov = OSSL_PROVIDER_load(NULL, "legacy");
    }
#endif

    unsigned char digest[EVP_MAX_MD_SIZE];
    uint32_t digSize = 0;
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    cx_err_t error = CX_INTERNAL_ERROR;

    if (md_ctx != NULL && EVP_DigestInit(md_ctx, md) == 1 &&
        EVP_DigestUpdate(md_ctx, (const void*)in, len) == 1 &&
        EVP_DigestFinal(md_ctx, digest, (unsigned int*)&digSize) == 1 &&
        digSize == (uint32_t)expected_size) {
        MSAN_UNPOISON(
            digest,
            digSize);  // Manually unpoison the bytes actually written by the
                       // OpenSSL digest, as it is not instrumented (installed
                       // from apt) and MSan can't see that it is initialized
                       // by EVP_DigestFinal.
        memcpy(out, digest, digSize);
        error = CX_OK;
    }

    if (md_ctx != NULL) {
        EVP_MD_CTX_free(md_ctx);
    }
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    if (prov) OSSL_PROVIDER_unload(prov);
#endif

    return error;
}
