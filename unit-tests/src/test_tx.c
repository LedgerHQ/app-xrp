#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <cmocka.h>
#include <string.h>

#include "../src/xrp/fmt.h"
#include "../src/xrp/xrp_helpers.h"
#include "../src/xrp/xrp_parse.h"
#include "cx.h"

parseContext_t parse_context;

static const char* testcases[] = {
    "../../tests/testcases/01-payment/01-basic.raw",
    "../../tests/testcases/01-payment/02-destination-tag.raw",
    "../../tests/testcases/01-payment/03-source-tag.raw",
    "../../tests/testcases/01-payment/04-both-tags.raw",
    "../../tests/testcases/01-payment/05-invoice-id.raw",
    "../../tests/testcases/01-payment/06-invoice-txn-ids-tags.raw",
    "../../tests/testcases/01-payment/07-issued-currency.raw",
    "../../tests/testcases/01-payment/08-issued-currency-max.raw",
    "../../tests/testcases/01-payment/09-issued-currency-min-partial.raw",
    "../../tests/testcases/01-payment/10-issued-currency-quality-partial.raw",
    "../../tests/testcases/01-payment/11-issued-currency-paths.raw",
    "../../tests/testcases/01-payment/12-issued-currency-conversion.raw",
    "../../tests/testcases/01-payment/13-issued-currency-e-notation.raw",
    "../../tests/testcases/01-payment/14-issued-currency-non-standard.raw",
    "../../tests/testcases/01-payment/15-issue-abc-currency.raw",
    "../../tests/testcases/01-payment/16-memos.raw",
    "../../tests/testcases/01-payment/17-multi-sign-parallel.raw",
    "../../tests/testcases/01-payment/18-multi-sign-serial.raw",
    //"../../tests/testcases/01-payment/19-really-stupid-tx.raw",
    "../../tests/testcases/01-payment/20-spoofed-xrp-currency.raw",
    "../../tests/testcases/01-payment/23-xrp-nonstandard-spoof.raw",
    "../../tests/testcases/02-set-regular-key/01-basic.raw",
    "../../tests/testcases/02-set-regular-key/02-delete.raw",
    "../../tests/testcases/02-set-regular-key/03-all-common-fields.raw",
    "../../tests/testcases/03-escrow-create/01-finish-after.raw",
    "../../tests/testcases/03-escrow-create/02-cancel-after.raw",
    "../../tests/testcases/03-escrow-create/03-both.raw",
    "../../tests/testcases/03-escrow-create/04-both-condition.raw",
    "../../tests/testcases/03-escrow-create/05-both-condition-destination.raw",
    "../../tests/testcases/03-escrow-create/06-all-common-fields.raw",
    "../../tests/testcases/04-escrow-finish/01-time-based.raw",
    "../../tests/testcases/04-escrow-finish/02-condition-based.raw",
    "../../tests/testcases/05-escrow-cancel/01-basic.raw",
    "../../tests/testcases/06-account-set/01-basic.raw",
    "../../tests/testcases/06-account-set/02-default-ripple.raw",
    "../../tests/testcases/06-account-set/03-deposit-auth.raw",
    "../../tests/testcases/06-account-set/04-disable-master.raw",
    "../../tests/testcases/06-account-set/05-disallow-xrp.raw",
    "../../tests/testcases/06-account-set/06-global-freeze.raw",
    "../../tests/testcases/06-account-set/07-no-freeze.raw",
    "../../tests/testcases/06-account-set/08-require-auth.raw",
    "../../tests/testcases/06-account-set/09-require-tag.raw",
    "../../tests/testcases/06-account-set/10-clear-account-txn-id.raw",
    "../../tests/testcases/07-check-cancel/01-basic.raw",
    "../../tests/testcases/08-check-cash/01-basic.raw",
    "../../tests/testcases/08-check-cash/02-amount.raw",
    "../../tests/testcases/08-check-cash/03-issued.raw",
    "../../tests/testcases/08-check-cash/04-issued-delivery-min.raw",
    "../../tests/testcases/09-check-create/01-basic.raw",
    "../../tests/testcases/09-check-create/02-issued.raw",
    "../../tests/testcases/10-deposit-preauth/01-basic.raw",
    "../../tests/testcases/10-deposit-preauth/02-unauthorize.raw",
    "../../tests/testcases/11-offer-cancel/01-basic.raw",
    "../../tests/testcases/12-offer-create/01-basic.raw",
    "../../tests/testcases/12-offer-create/02-passive.raw",
    "../../tests/testcases/12-offer-create/03-immediate-or-cancel.raw",
    "../../tests/testcases/12-offer-create/04-fill-or-kill.raw",
    "../../tests/testcases/12-offer-create/05-sell.raw",
    "../../tests/testcases/12-offer-create/06-combo.raw",
    "../../tests/testcases/13-payment-channel-claim/01-basic.raw",
    "../../tests/testcases/13-payment-channel-claim/02-renew.raw",
    "../../tests/testcases/13-payment-channel-claim/03-close.raw",
    "../../tests/testcases/14-payment-channel-create/01-basic.raw",
    "../../tests/testcases/15-payment-channel-fund/01-basic.raw",
    "../../tests/testcases/16-signer-list-set/01-basic.raw",
    "../../tests/testcases/16-signer-list-set/02-delete.raw",
    "../../tests/testcases/17-trust-set/01-basic.raw",
    "../../tests/testcases/17-trust-set/02-quality.raw",
    "../../tests/testcases/17-trust-set/03-authorize.raw",
    "../../tests/testcases/17-trust-set/04-no-rippling.raw",
    "../../tests/testcases/17-trust-set/05-rippling.raw",
    "../../tests/testcases/17-trust-set/06-freeze.raw",
    "../../tests/testcases/17-trust-set/07-unfreeze.raw",
    "../../tests/testcases/17-trust-set/08-non-standard-currency.raw",
    "../../tests/testcases/17-trust-set/09-remove.raw",
    "../../tests/testcases/18-arrays/01-basic.raw",
    "../../tests/testcases/18-arrays/02-multiple.raw",
    "../../tests/testcases/18-arrays/03-not-last.raw",
    "../../tests/testcases/19-nftoken-mint/01-basic.raw",
    "../../tests/testcases/19-nftoken-mint/02-burnable.raw",
    "../../tests/testcases/19-nftoken-mint/03-only-xrp.raw",
    "../../tests/testcases/19-nftoken-mint/04-transferable.raw",
    "../../tests/testcases/20-nftoken-burn/01-basic.raw",
    "../../tests/testcases/21-nftoken-create-offer/01-sell.raw",
    "../../tests/testcases/21-nftoken-create-offer/02-sell-destination.raw",
    "../../tests/testcases/21-nftoken-create-offer/03-buy.raw",
    "../../tests/testcases/21-nftoken-create-offer/04-buy-expiration.raw",
    "../../tests/testcases/22-nftoken-cancel-offer/01-basic.raw",
    "../../tests/testcases/23-nftoken-accept-offer/01-basic.raw",
    "../../tests/testcases/23-nftoken-accept-offer/02-broker.raw",
    "../../tests/testcases/24-clawback/01-basic.raw",
    "../../tests/testcases/25-amm-create/01-basic.raw",
    "../../tests/testcases/26-amm-deposit/01-one-sided.raw",
    "../../tests/testcases/26-amm-deposit/02-two-sided.raw",
    "../../tests/testcases/26-amm-deposit/03-amount-lp-out.raw",
    "../../tests/testcases/26-amm-deposit/04-lp-out.raw",
    "../../tests/testcases/27-amm-withdraw/01-one-sided.raw",
    "../../tests/testcases/27-amm-withdraw/02-two-sided.raw",
    "../../tests/testcases/27-amm-withdraw/03-amount-lp-out.raw",
    "../../tests/testcases/27-amm-withdraw/04-lp-out.raw",
    "../../tests/testcases/28-amm-bid/01-basic.raw",
    "../../tests/testcases/28-amm-bid/02-min-max.raw",
    "../../tests/testcases/29-amm-vote/01-basic.raw",
    NULL,
};

// Transactions that must be rejected by the parser (parse_tx returns non-zero).
static const char* negative_testcases[] = {
    // Currency with first byte 0x00 and XRP ticker: invalid per spec —
    // 0x00 prefix is reserved for standard codes and "XRP" is disallowed.
    "../../tests/testcases/01-payment/22-xrp-reserved-ticker.raw",
    // Composite path step type 0x11 (account|currency bitmask) is explicitly
    // forbidden by the XRPL spec; only 0x01, 0x10, 0x20, 0x30 are valid.
    "../../tests/testcases/01-payment/24-invalid-path-step-type.raw",
    NULL,
};

static uint8_t* load_transaction_data(const char* filename, size_t* size) {
    uint8_t* data = NULL;

    FILE* f = fopen(filename, "rb");
    assert_non_null(f);

    assert_int_equal(fseek(f, 0, SEEK_END), 0);
    long filesize_long = ftell(f);
    assert_true(filesize_long >= 0);
    assert_int_equal(fseek(f, 0, SEEK_SET), 0);

    size_t filesize = (size_t)filesize_long;
    if (filesize > 0) {
        data = malloc(filesize);
        assert_non_null(data);
        assert_true(fread(data, 1, filesize, f) == filesize);
    }
    *size = filesize;
    fclose(f);

    return data;
}

static void update_title(field_t* field, field_name_t* title) {
    const char* name = resolve_field_name(field);
    strncpy(title->buf, name, sizeof(title->buf));
    title->buf[sizeof(title->buf) - 1] = '\x00';

    size_t len = strlen(title->buf);
    if (field->array_info.type == ARRAY_PATHSET) {
        snprintf(title->buf + len, sizeof(title->buf) - len, " [P%d: S%d]",
                 field->array_info.index1, field->array_info.index2);
    } else if (field->array_info.type != ARRAY_NONE) {
        snprintf(title->buf + len, sizeof(title->buf) - len, " [%d]",
                 field->array_info.index1);
    }
}

static void update_value(field_t* field, field_value_t* value) {
    format_field(field, value);
}

static void get_result_filename(const char* filename, char* path, size_t size) {
    strncpy(path, filename, size);

    char* ext = strstr(path, ".raw");
    assert_non_null(ext);
    memcpy(ext, ".txt", 4);
}

static void generate_expected_result(const char* filename,
                                     parseResult_t* transaction) {
    char path[1024];
    get_result_filename(filename, path, sizeof(path));

    FILE* fp = fopen(path, "w");
    assert_non_null(fp);

    for (int i = 0; i < transaction->num_fields; ++i) {
        field_t* field = &transaction->fields[i];
        field_name_t field_name;
        field_value_t field_value;
        update_title(field, &field_name);
        update_value(field, &field_value);
        fprintf(fp, "%s; %s\n", field_name.buf, field_value.buf);
    }

    fclose(fp);
}

static void check_transaction_results(const char* filename,
                                      parseResult_t* transaction) {
    printf("[*] %s\n", filename);
    char path[1024];
    get_result_filename(filename, path, sizeof(path));

    FILE* fp = fopen(path, "r");
    assert_non_null(fp);

    for (int i = 0; i < transaction->num_fields; ++i) {
        field_t* field = &transaction->fields[i];
        field_name_t field_name;
        field_value_t field_value;
        update_title(field, &field_name);
        update_value(field, &field_value);

        char line[4096];
        assert_non_null(fgets(line, sizeof(line), fp));

        char* expected_title = line;
        char* expected_value = strstr(line, "; ");
        assert_non_null(expected_value);

        *expected_value = '\x00';
        assert_string_equal(expected_title, field_name.buf);

        expected_value += 2;
        char* p = strchr(expected_value, '\n');
        if (p != NULL) {
            *p = '\x00';
        }
        assert_string_equal(field_value.buf, expected_value);
    }

    fclose(fp);
}

static void test_tx(const char* filename) {
    size_t size;
    uint8_t* data = load_transaction_data(filename, &size);

    memset(&parse_context, 0, sizeof(parse_context));
    parse_context.data = data;
    parse_context.length = size;
    assert_int_equal(parse_tx(&parse_context), 0);

    parseResult_t* transaction = &parse_context.result;
    if (false) {
        generate_expected_result(filename, transaction);
    }
    check_transaction_results(filename, transaction);

    free(data);
}

void test_transactions(void** state) {
    (void)state;

    for (const char** testcase = testcases; *testcase != NULL; testcase++) {
        test_tx(*testcase);
    }
}

void test_invalid_transactions(void** state) {
    (void)state;

    for (const char** testcase = negative_testcases; *testcase != NULL;
         testcase++) {
        size_t size;
        uint8_t* data = load_transaction_data(*testcase, &size);

        memset(&parse_context, 0, sizeof(parse_context));
        parse_context.data = data;
        parse_context.length = size;
        assert_int_not_equal(parse_tx(&parse_context), 0);

        free(data);
    }
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_transactions),
        cmocka_unit_test(test_invalid_transactions),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
