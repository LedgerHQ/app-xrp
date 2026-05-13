#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <cmocka.h>

#include "cx.h"
#include "../src/xrp/xrp_parse.h"
#include "../src/xrp/xrp_helpers.h"

parseContext_t parse_context;

void test_address(void **state) {
    (void) state;

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

void test_print_amount(void **state) {
    (void) state;

    char buf[128];
    assert_int_equal(xrp_print_amount(123456, buf, sizeof(buf)), 0);
    assert_string_equal(buf, "XRP 0.123456");

    assert_int_equal(xrp_print_amount(0xffffffffffffffff, buf, sizeof(buf)), -1);

    uint64_t amount = 0x8ac7230489e7ffff;
    assert_int_equal(xrp_print_amount(amount, buf, sizeof(buf)), 0);
    assert_string_equal(buf, "XRP 9999999999999.999999");

    assert_int_equal(xrp_print_amount(amount + 1, buf, sizeof(buf)), -1);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_address),
        cmocka_unit_test(test_print_amount),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
