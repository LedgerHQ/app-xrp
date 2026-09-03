// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
// clang-format on
#include <string.h>

#include "../src/xrp/xrp_helpers.h"
#include "../src/xrp/xrp_parse.h"
#include "cx.h"

parseContext_t parse_context;

void test_address(void** state) {
    (void)state;

    xrp_address_t address;

    memset(&address, 'a', sizeof(address));

    cx_ecfp_public_key_t public_key = {
        .curve = CX_CURVE_SECP256K1,
        .W_len = 0x41,
        .W = "\x04\x0f\x14\x58\x1f\x0e\xab\x34\xe5\xc9\x58\xbf\xa0\x40\x1b\x0f"
             "\x26\x5c\x16\x61\x70\xac\xce\x22\xd1\x22\x48\x0b\x33\x51\x73\x38"
             "\x65\x61\xb4\xc5\xbb\xf1\xcc\x70\x24\xe5\x4d\xa1\x49\xc2\x08\xd1"
             "\xd9\x75\x39\x7b\xa2\x7f\xf5\x0a\x85\x63\x3c\xcd\x31\xc5\xc6\x33"
             "\x70"};
    get_address(&public_key, &address);

    assert_string_equal(address.buf, "r3wi8h8ZYuJDm4TwYaAFAwR8ZwnPSiN3CC");
}

void test_public_key_to_base58_requires_exactly_one_source(void** state) {
    (void)state;

    xrp_pubkey_t pubkey;
    memset(&pubkey, 0x02, sizeof(pubkey));
    xrp_account_t account;
    memset(&account, 0x03, sizeof(account));
    xrp_address_t out;

    // Neither pubkey nor account given: must not silently encode whatever
    // stale bytes were already on the stack.
    assert_int_equal(xrp_public_key_to_encoded_base58(NULL, NULL, &out, 0), 0);

    // Both given: must not silently let account overwrite the pubkey hash.
    assert_int_equal(
        xrp_public_key_to_encoded_base58(&pubkey, &account, &out, 0), 0);

    // Exactly one given, either way: must still work normally.
    assert_true(xrp_public_key_to_encoded_base58(&pubkey, NULL, &out, 0) > 0);
    assert_true(xrp_public_key_to_encoded_base58(NULL, &account, &out, 0) > 0);
}

void test_cx_hash_no_throw_rejects_undersized_buffer(void** state) {
    (void)state;

    cx_sha256_t sha256_hash;
    cx_sha256_init(&sha256_hash);
    unsigned char in[] = "some data";
    unsigned char canary_guard[36];
    memset(canary_guard, 0xAA, sizeof(canary_guard));

    // Too small: must error out without writing anything, canary untouched.
    assert_int_equal(cx_hash_no_throw(&sha256_hash.header, CX_LAST, in,
                                      sizeof(in), canary_guard, 31),
                     (int)CX_INVALID_PARAMETER_SIZE);
    for (size_t i = 0; i < sizeof(canary_guard); i++) {
        assert_int_equal(canary_guard[i], 0xAA);
    }

    // Exact size: succeeds and writes exactly the digest.
    unsigned char exact[32];
    cx_sha256_init(&sha256_hash);
    assert_int_equal(cx_hash_no_throw(&sha256_hash.header, CX_LAST, in,
                                      sizeof(in), exact, sizeof(exact)),
                     CX_OK);

    // Oversized: succeeds, and matches the exact-size digest bit for bit.
    unsigned char oversized[40];
    memset(oversized, 0xAA, sizeof(oversized));
    cx_sha256_init(&sha256_hash);
    assert_int_equal(cx_hash_no_throw(&sha256_hash.header, CX_LAST, in,
                                      sizeof(in), oversized, sizeof(oversized)),
                     CX_OK);
    assert_memory_equal(oversized, exact, sizeof(exact));

    cx_ripemd160_t ripemd_hash;
    cx_ripemd160_init(&ripemd_hash);
    assert_int_equal(cx_hash_no_throw(&ripemd_hash.header, CX_LAST, in,
                                      sizeof(in), canary_guard, 19),
                     (int)CX_INVALID_PARAMETER_SIZE);
}

void test_print_amount(void** state) {
    (void)state;

    char buf[128];
    assert_int_equal(xrp_print_amount(123456, buf, sizeof(buf)), 0);
    assert_string_equal(buf, "XRP 0.123456");

    assert_int_equal(xrp_print_amount(0xffffffffffffffff, buf, sizeof(buf)),
                     -1);

    uint64_t amount = 0x8ac7230489e7ffff;
    assert_int_equal(xrp_print_amount(amount, buf, sizeof(buf)), 0);
    assert_string_equal(buf, "XRP 9999999999999.999999");

    assert_int_equal(xrp_print_amount(amount + 1, buf, sizeof(buf)), -1);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_address),
        cmocka_unit_test(test_public_key_to_base58_requires_exactly_one_source),
        cmocka_unit_test(test_cx_hash_no_throw_rejects_undersized_buffer),
        cmocka_unit_test(test_print_amount),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
