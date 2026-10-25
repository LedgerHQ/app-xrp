// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
// clang-format on
#include <string.h>

#include "../src/xrp/amount.h"
#include "../src/xrp/fmt.h"
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

static void encode_number(uint8_t* data, uint64_t mantissa, uint32_t exponent) {
    for (int i = 0; i < 8; i++) {
        data[i] = (uint8_t)(mantissa >> (56 - 8 * i));
    }
    for (int i = 0; i < 4; i++) {
        data[8 + i] = (uint8_t)(exponent >> (24 - 8 * i));
    }
}

static void check_number(uint64_t mantissa, uint32_t exponent,
                         const char* expected) {
    uint8_t data[NUMBER_LEN];
    encode_number(data, mantissa, exponent);

    field_t field;
    memset(&field, 0, sizeof(field));
    field.data_type = STI_NUMBER;
    field.data.ptr = data;
    field.length = NUMBER_LEN;

    field_value_t value;
    memset(&value, 0, sizeof(value));
    number_formatter(&field, &value);
    assert_string_equal(value.buf, expected);
}

void test_print_number(void** state) {
    (void)state;

    // Zero, whatever the exponent.
    check_number(0, 0, "0");
    check_number(0, 0x7FFFFFFF, "0");

    // rippled serializes a Number with a 19-digit mantissa.
    check_number(2750000000000000000, (uint32_t)-12, "2750000");
    check_number(1250000000000000000, (uint32_t)-18, "1.25");
    check_number(5000000000000000000, (uint32_t)-19, "0.5");
    check_number(1000000000000000000, (uint32_t)-21, "0.001");
    check_number(1000, (uint32_t)-3, "1");
    check_number((uint64_t)-4200000000000000000, (uint32_t)-17, "-42");

    // The mantissa's extremes.
    check_number(0x7FFFFFFFFFFFFFFF, 0, "9223372036854775807");
    check_number(0x8000000000000000, 0, "-9223372036854775808");

    // Plain decimals within an issued-currency amount's exponent range,
    // the exponent shown beyond it.
    char expected[128];
    memset(expected, '0', sizeof(expected));
    expected[0] = '1';
    expected[81] = '\x00';
    check_number(1, 80, expected);
    check_number(1, 81, "1e81");

    memset(expected, '0', sizeof(expected));
    expected[1] = '.';
    expected[97] = '1';
    expected[98] = '\x00';
    check_number(1, (uint32_t)-96, expected);
    check_number(1, (uint32_t)-97, "1e-97");

    // Stripping trailing zeros at the 32-bit exponent's edges must not wrap.
    check_number(10, 0x7FFFFFFF, "1e2147483648");
    check_number((uint64_t)-10, 0x7FFFFFFF, "-1e2147483648");
    check_number(1, 0x80000000, "1e-2147483648");

    // The longest plain decimal, 100 characters, fits in a field value.
    memset(expected, '0', sizeof(expected));
    memcpy(expected, "-9223372036854775808", 20);
    expected[100] = '\x00';
    check_number(0x8000000000000000, 80, expected);
}

static bool in_range(uint64_t mantissa, uint32_t exponent) {
    uint8_t data[NUMBER_LEN];
    encode_number(data, mantissa, exponent);
    return is_number_in_range(data);
}

// The parser refuses a Number not shown as a plain decimal, in its base
// units (exponents -96..80) or in XRP, 10^6 drops (-90..80).
void test_number_in_range(void** state) {
    (void)state;

    assert_true(in_range(0, 0));
    assert_true(in_range(0, 0x80000000));
    assert_true(in_range(1, 80));
    assert_false(in_range(1, 81));
    assert_true(in_range(1, (uint32_t)-90));
    assert_false(in_range(1, (uint32_t)-91));

    // The exponent counts the mantissa's trailing zeros.
    assert_false(in_range(10, 80));
    assert_true(in_range(1000, (uint32_t)-93));
    assert_false(in_range(1000000000000000000, (uint32_t)-40018));
    assert_false(in_range((uint64_t)-1, (uint32_t)-91));

    // At the 32-bit exponent's edges, without wrapping.
    assert_false(in_range(10, 0x7FFFFFFF));
    assert_false(in_range(1, 0x80000000));
}

// A Number longer than its buffer is refused, never cut short.
void test_print_number_too_long(void** state) {
    (void)state;

    uint8_t data[NUMBER_LEN];
    char buf[8];

    encode_number(data, 1250000000000000000, (uint32_t)-18);
    assert_int_equal(print_number(buf, 5, data, 0), 0);
    assert_string_equal(buf, "1.25");
    assert_int_equal(print_number(buf, 4, data, 0), -1);
    assert_string_equal(buf, "");

    encode_number(data, 1, 81);
    assert_int_equal(print_number(buf, 5, data, 0), 0);
    assert_string_equal(buf, "1e81");
    assert_int_equal(print_number(buf, 4, data, 0), -1);
    assert_string_equal(buf, "");

    encode_number(data, (uint64_t)-42, 0);
    assert_int_equal(print_number(buf, 4, data, 0), 0);
    assert_string_equal(buf, "-42");
    assert_int_equal(print_number(buf, 3, data, 0), -1);
    assert_string_equal(buf, "");

    encode_number(data, 0, 0);
    assert_int_equal(print_number(buf, 2, data, 0), 0);
    assert_string_equal(buf, "0");
    assert_int_equal(print_number(buf, 1, data, 0), -1);
    assert_string_equal(buf, "");
    assert_int_equal(print_number(buf, 0, data, 0), -1);
}

static void check_duration(uint8_t id, uint32_t seconds, const char* expected) {
    field_t field;
    memset(&field, 0, sizeof(field));
    field.data_type = STI_UINT32;
    field.id = id;
    field.data.u32 = seconds;

    field_value_t value;
    format_field(&field, &value);
    assert_string_equal(value.buf, expected);
}

// A loan's payment interval and grace period, exact, never rounded.
void test_print_duration(void** state) {
    (void)state;

    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 0, "0 seconds");
    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 1, "1 second");
    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 59, "59 seconds");
    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 60, "1 minute");
    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 3661,
                   "1 hour 1 minute 1 second");
    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 86399,
                   "23 hours 59 minutes 59 seconds");
    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 86400, "1 day");
    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 86401, "1 day 1 second");
    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 259200, "3 days");
    check_duration(XRP_UINT32_PAYMENT_INTERVAL, 2592000, "30 days");
    check_duration(XRP_UINT32_GRACE_PERIOD, 604800, "7 days");
    check_duration(XRP_UINT32_GRACE_PERIOD, UINT32_MAX,
                   "49710 days 6 hours 28 minutes 15 seconds");

    // A payment channel's SettleDelay keeps its own format.
    check_duration(XRP_UINT32_SETTLE_DELAY, 86400, "86400 s");
}

static void check_percentage(field_type_t type, uint8_t id, uint32_t value,
                             const char* expected) {
    field_t field;
    memset(&field, 0, sizeof(field));
    field.data_type = type;
    field.id = id;
    if (type == STI_UINT16) {
        field.data.u16 = (uint16_t)value;
    } else {
        field.data.u32 = value;
    }

    field_value_t value_str;
    format_field(&field, &value_str);
    assert_string_equal(value_str.buf, expected);
}

// Both scales: a TransferRate or Quality in billionths over 1, an XLS-66
// rate in tenths of a basis point.
void test_print_percentage(void** state) {
    (void)state;

    check_percentage(STI_UINT32, XRP_UINT32_TRANSFER_RATE, 0, "0 %");
    check_percentage(STI_UINT32, XRP_UINT32_TRANSFER_RATE, 999999999,
                     "Invalid value");
    check_percentage(STI_UINT32, XRP_UINT32_TRANSFER_RATE, 1000000000, "0 %");
    check_percentage(STI_UINT32, XRP_UINT32_TRANSFER_RATE, 1002000000, "0.2 %");
    check_percentage(STI_UINT32, XRP_UINT32_TRANSFER_RATE, 1234567891,
                     "23.4567891 %");
    check_percentage(STI_UINT32, XRP_UINT32_TRANSFER_RATE, UINT32_MAX,
                     "329.4967295 %");
    check_percentage(STI_UINT32, XRP_UINT32_QUALITY_IN, 0, "100 %");
    check_percentage(STI_UINT32, XRP_UINT32_QUALITY_IN, 1, "0.0000001 %");

    check_percentage(STI_UINT32, XRP_UINT32_INTEREST_RATE, 0, "0 %");
    check_percentage(STI_UINT32, XRP_UINT32_INTEREST_RATE, 1, "0.001 %");
    check_percentage(STI_UINT32, XRP_UINT32_INTEREST_RATE, 9250, "9.25 %");
    check_percentage(STI_UINT32, XRP_UINT32_INTEREST_RATE, 100000, "100 %");
    check_percentage(STI_UINT32, XRP_UINT32_INTEREST_RATE, UINT32_MAX,
                     "4294967.295 %");
    check_percentage(STI_UINT16, XRP_UINT16_MANAGEMENT_FEE_RATE, 10000, "10 %");
    check_percentage(STI_UINT16, XRP_UINT16_MANAGEMENT_FEE_RATE, 65535,
                     "65.535 %");
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_address),
        cmocka_unit_test(test_public_key_to_base58_requires_exactly_one_source),
        cmocka_unit_test(test_cx_hash_no_throw_rejects_undersized_buffer),
        cmocka_unit_test(test_print_amount),
        cmocka_unit_test(test_print_number),
        cmocka_unit_test(test_print_number_too_long),
        cmocka_unit_test(test_number_in_range),
        cmocka_unit_test(test_print_duration),
        cmocka_unit_test(test_print_percentage),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
