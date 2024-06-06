#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "cx.h"
#include "../src/xrp/xrp_parse.h"
#include "../src/xrp/xrp_helpers.h"
#include "../src/xrp/fmt.h"

parseContext_t parse_context;

static const char *testcases[] = {
    "../testcases/01-payment/01-basic.raw",
    "../testcases/01-payment/02-destination-tag.raw",
    "../testcases/01-payment/03-source-tag.raw",
    "../testcases/01-payment/04-both-tags.raw",
    "../testcases/01-payment/05-invoice-id.raw",
    "../testcases/01-payment/06-invoice-txn-ids-tags.raw",
    "../testcases/01-payment/07-issued-currency.raw",
    "../testcases/01-payment/08-issued-currency-max.raw",
    "../testcases/01-payment/09-issued-currency-min-partial.raw",
    "../testcases/01-payment/10-issued-currency-quality-partial.raw",
    "../testcases/01-payment/11-issued-currency-paths.raw",
    "../testcases/01-payment/12-issued-currency-conversion.raw",
    "../testcases/01-payment/13-issued-currency-e-notation.raw",
    "../testcases/01-payment/14-issued-currency-non-standard.raw",
    "../testcases/01-payment/15-issue-abc-currency.raw",
    "../testcases/01-payment/16-memos.raw",
    "../testcases/01-payment/17-multi-sign-parallel.raw",
    "../testcases/01-payment/18-multi-sign-serial.raw",
    //"../testcases/01-payment/19-really-stupid-tx.raw",
    "../testcases/02-set-regular-key/01-basic.raw",
    "../testcases/02-set-regular-key/02-delete.raw",
    "../testcases/02-set-regular-key/03-all-common-fields.raw",
    "../testcases/03-escrow-create/01-finish-after.raw",
    "../testcases/03-escrow-create/02-cancel-after.raw",
    "../testcases/03-escrow-create/03-both.raw",
    "../testcases/03-escrow-create/04-both-condition.raw",
    "../testcases/03-escrow-create/05-both-condition-destination.raw",
    "../testcases/03-escrow-create/06-all-common-fields.raw",
    "../testcases/04-escrow-finish/01-time-based.raw",
    "../testcases/04-escrow-finish/02-condition-based.raw",
    "../testcases/05-escrow-cancel/01-basic.raw",
    "../testcases/06-account-set/01-basic.raw",
    "../testcases/06-account-set/02-default-ripple.raw",
    "../testcases/06-account-set/03-deposit-auth.raw",
    "../testcases/06-account-set/04-disable-master.raw",
    "../testcases/06-account-set/05-disallow-xrp.raw",
    "../testcases/06-account-set/06-global-freeze.raw",
    "../testcases/06-account-set/07-no-freeze.raw",
    "../testcases/06-account-set/08-require-auth.raw",
    "../testcases/06-account-set/09-require-tag.raw",
    "../testcases/06-account-set/10-clear-account-txn-id.raw",
    "../testcases/07-check-cancel/01-basic.raw",
    "../testcases/08-check-cash/01-basic.raw",
    "../testcases/08-check-cash/02-amount.raw",
    "../testcases/08-check-cash/03-issued.raw",
    "../testcases/08-check-cash/04-issued-delivery-min.raw",
    "../testcases/09-check-create/01-basic.raw",
    "../testcases/09-check-create/02-issued.raw",
    "../testcases/10-deposit-preauth/01-basic.raw",
    "../testcases/10-deposit-preauth/02-unauthorize.raw",
    "../testcases/11-offer-cancel/01-basic.raw",
    "../testcases/12-offer-create/01-basic.raw",
    "../testcases/12-offer-create/02-passive.raw",
    "../testcases/12-offer-create/03-immediate-or-cancel.raw",
    "../testcases/12-offer-create/04-fill-or-kill.raw",
    "../testcases/12-offer-create/05-sell.raw",
    "../testcases/12-offer-create/06-combo.raw",
    "../testcases/13-payment-channel-claim/01-basic.raw",
    "../testcases/13-payment-channel-claim/02-renew.raw",
    "../testcases/13-payment-channel-claim/03-close.raw",
    "../testcases/14-payment-channel-create/01-basic.raw",
    "../testcases/15-payment-channel-fund/01-basic.raw",
    "../testcases/16-signer-list-set/01-basic.raw",
    "../testcases/16-signer-list-set/02-delete.raw",
    "../testcases/17-trust-set/01-basic.raw",
    "../testcases/17-trust-set/02-quality.raw",
    "../testcases/17-trust-set/03-authorize.raw",
    "../testcases/17-trust-set/04-no-rippling.raw",
    "../testcases/17-trust-set/05-rippling.raw",
    "../testcases/17-trust-set/06-freeze.raw",
    "../testcases/17-trust-set/07-unfreeze.raw",
    "../testcases/17-trust-set/08-non-standard-currency.raw",
    "../testcases/17-trust-set/09-remove.raw",
    "../testcases/18-arrays/01-basic.raw",
    "../testcases/18-arrays/02-multiple.raw",
    "../testcases/18-arrays/03-not-last.raw",
    "../testcases/19-nftoken-mint/01-basic.raw",
    "../testcases/19-nftoken-mint/02-burnable.raw",
    "../testcases/19-nftoken-mint/03-only-xrp.raw",
    "../testcases/19-nftoken-mint/04-transferable.raw",
    "../testcases/20-nftoken-burn/01-basic.raw",
    "../testcases/21-nftoken-create-offer/01-sell.raw",
    "../testcases/21-nftoken-create-offer/02-sell-destination.raw",
    "../testcases/21-nftoken-create-offer/03-buy.raw",
    "../testcases/21-nftoken-create-offer/04-buy-expiration.raw",
    "../testcases/22-nftoken-cancel-offer/01-basic.raw",
    "../testcases/23-nftoken-accept-offer/01-basic.raw",
    "../testcases/23-nftoken-accept-offer/02-broker.raw",
    "../testcases/24-clawback/01-basic.raw",
    "../testcases/25-amm-create/01-basic.raw",
    "../testcases/26-amm-deposit/01-one-sided.raw",
    "../testcases/26-amm-deposit/02-two-sided.raw",
    "../testcases/26-amm-deposit/03-amount-lp-out.raw",
    "../testcases/26-amm-deposit/04-lp-out.raw",
    "../testcases/27-amm-withdraw/01-one-sided.raw",
    "../testcases/27-amm-withdraw/02-two-sided.raw",
    "../testcases/27-amm-withdraw/03-amount-lp-out.raw",
    "../testcases/27-amm-withdraw/04-lp-out.raw",
    "../testcases/28-amm-bid/01-basic.raw",
    "../testcases/28-amm-bid/02-min-max.raw",
    "../testcases/29-amm-vote/01-basic.raw",
    NULL,
};

static uint8_t *load_transaction_data(const char *filename, size_t *size) {
    uint8_t *data;

    FILE *f = fopen(filename, "rb");
    assert_non_null(f);

    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = malloc(filesize);
    assert_non_null(data);
    assert_int_equal(fread(data, 1, filesize, f), filesize);
    *size = filesize;
    fclose(f);

    return data;
}

static void update_title(field_t *field, field_name_t *title) {
    const char *name = resolve_field_name(field);
    strncpy(title->buf, name, sizeof(title->buf));
    title->buf[sizeof(title->buf) - 1] = '\x00';

    size_t len = strlen(title->buf);
    if (field->array_info.type == ARRAY_PATHSET) {
        snprintf(title->buf + len,
                 sizeof(title->buf) - len,
                 " [P%d: S%d]",
                 field->array_info.index1,
                 field->array_info.index2);
    } else if (field->array_info.type != ARRAY_NONE) {
        snprintf(title->buf + len, sizeof(title->buf) - len, " [%d]", field->array_info.index1);
    }
}

static void update_value(field_t *field, field_value_t *value) {
    format_field(field, value);
}

static void get_result_filename(const char *filename, char *path, size_t size) {
    strncpy(path, filename, size);

    char *ext = strstr(path, ".raw");
    assert_non_null(ext);
    memcpy(ext, ".txt", 4);
}

static void generate_expected_result(const char *filename, parseResult_t *transaction) {
    char path[1024];
    get_result_filename(filename, path, sizeof(path));

    FILE *fp = fopen(path, "w");
    assert_non_null(fp);

    for (int i = 0; i < transaction->num_fields; ++i) {
        field_t *field = &transaction->fields[i];
        field_name_t field_name;
        field_value_t field_value;
        update_title(field, &field_name);
        update_value(field, &field_value);
        fprintf(fp, "%s; %s\n", field_name.buf, field_value.buf);
    }

    fclose(fp);
}

static void check_transaction_results(const char *filename, parseResult_t *transaction) {
    printf("[*] %s\n", filename);
    char path[1024];
    get_result_filename(filename, path, sizeof(path));

    FILE *fp = fopen(path, "r");
    assert_non_null(fp);

    for (int i = 0; i < transaction->num_fields; ++i) {
        field_t *field = &transaction->fields[i];
        field_name_t field_name;
        field_value_t field_value;
        update_title(field, &field_name);
        update_value(field, &field_value);

        char line[4096];
        assert_non_null(fgets(line, sizeof(line), fp));

        char *expected_title = line;
        char *expected_value = strstr(line, "; ");
        assert_non_null(expected_value);

        *expected_value = '\x00';
        assert_string_equal(expected_title, field_name.buf);

        expected_value += 2;
        char *p = strchr(expected_value, '\n');
        if (p != NULL) {
            *p = '\x00';
        }
        assert_string_equal(expected_title, field_name.buf);
        assert_string_equal(field_value.buf, expected_value);
    }

    fclose(fp);
}

static void test_tx(const char *filename) {
    size_t size;
    uint8_t *data = load_transaction_data(filename, &size);

    memset(&parse_context, 0, sizeof(parse_context));
    parse_context.data = data;
    parse_context.length = size;
    assert_int_equal(parse_tx(&parse_context), 0);

    parseResult_t *transaction = &parse_context.result;
    if (false) {
        generate_expected_result(filename, transaction);
    }
    check_transaction_results(filename, transaction);

    free(data);
}

void test_transactions(void **state) {
    (void) state;

    for (const char **testcase = testcases; *testcase != NULL; testcase++) {
        test_tx(*testcase);
    }
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_transactions),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
