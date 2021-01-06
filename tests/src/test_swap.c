#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <cmocka.h>

#include "cx.h"
#include "../src/swap/handle_check_address.h"
#include "../src/xrp/xrp_helpers.h"

void test_check_address(void **state) {
    (void) state;

    check_address_parameters_t params = {
        .address_to_check = "r3wi8h8ZYuJDm4TwYaAFAwR8ZwnPSiN3CC",
        .address_parameters = (uint8_t *)"\x05\x80\x00\x00\x2c\x80\x00\x00\x90\x80\x00\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00",
        .address_parameters_length = 21, /* XXX: never used */
    };

    /* The public key is hardcoded. Indeed, get_public_key is not available from tests because of
     * exceptions and cryptographic calls. */
    cx_ecfp_public_key_t public_key = {
        .curve = CX_CURVE_256K1,
        .W_len = 33,
        .W = "\x02\x0f\x14\x58\x1f\x0e\xab\x34\xe5\xc9\x58\xbf\xa0\x40\x1b\x0f\x26\x5c\x16\x61\x70"
             "\xac"
             "\xce\x22\xd1\x22\x48\x0b\x33\x51\x73\x38\x65",
    };

    uint32_t bip32_path_parsed[MAX_BIP32_PATH];
    uint8_t *bip32_path_ptr = params.address_parameters;
    uint8_t bip32_path_length = *(bip32_path_ptr++);
    assert_int_equal(
        parse_bip32_path(bip32_path_ptr, bip32_path_length, bip32_path_parsed, MAX_BIP32_PATH),
        1);

    xrp_address_t address;
    get_address(&public_key, &address);

    assert_string_equal(address.buf, params.address_to_check);
}

void test_get_printable_amount(void **state) {
    (void) state;

    get_printable_amount_parameters_t params = {
        .amount = (uint8_t *) "\x00\x04\xd2",
        .amount_length = 3,
    };

    uint64_t amount;
    assert_int_equal(swap_str_to_u64(params.amount, params.amount_length, &amount), 1);
    assert_int_equal(amount, 0x4d2);
    assert_int_equal(
        xrp_print_amount(amount, params.printable_amount, sizeof(params.printable_amount)),
        0);
    assert_string_equal(params.printable_amount, "XRP 0.001234");
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_check_address),
        cmocka_unit_test(test_get_printable_amount),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
