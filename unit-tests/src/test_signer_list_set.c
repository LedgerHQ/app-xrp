#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <cmocka.h>
#include <string.h>

#include "../src/xrp/xrp_parse.h"
#include "cx.h"

parseContext_t parse_context;

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

// SignerListSet carrying 27 SignerEntries (4 base + 27 * 2 = 58 display
// fields). A SignerEntry expands into only 2 fields (Account, SignerWeight), so
// the raised MAX_ARRAY_LEN (27) permits more entries here than the multisign
// Signers array, which remains field-budget-bound at 18 signers.
void test_signer_list_set_max_entries(void** state) {
    (void)state;

    size_t size;
    uint8_t* data = load_transaction_data(
        "../testcases/30-multi-sign/03-signer-list-set-27-entries.raw", &size);

    memset(&parse_context, 0, sizeof(parse_context));
    parse_context.data = data;
    parse_context.length = size;

    assert_int_equal(parse_tx(&parse_context), 0);
    assert_int_equal(parse_context.result.num_fields, 4 + 27 * 2);

    free(data);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_signer_list_set_max_entries),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
