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
    uint32_t digSize = 0;
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    OSSL_PROVIDER* prov = NULL;
#endif

    if (hash->algo == CX_SHA256) {
        EVP_DigestInit(md_ctx, EVP_sha256());
    } else if (hash->algo == CX_RIPEMD160) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
        prov = OSSL_PROVIDER_load(NULL, "legacy");
#endif
        EVP_DigestInit(md_ctx, EVP_ripemd160());
    } else {
        abort();
    }

    EVP_DigestUpdate(md_ctx, (const void*)in, len);
    EVP_DigestFinal(md_ctx, out, (unsigned int*)&digSize);
    MSAN_UNPOISON(
        out,
        out_len);  // Manually unpoison output buffer of the OpenSSL digest as
                   // it is not instrumented (installed from apt) and MSan can't
                   // see that it is initialized by EVP_DigestFinal.
    EVP_MD_CTX_free(md_ctx);
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    if (prov) OSSL_PROVIDER_unload(prov);
#endif

    if (digSize != out_len) {
        abort();
    }

    return 0;
}
