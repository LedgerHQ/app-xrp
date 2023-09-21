#include <malloc.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "cx.h"
#include "../src/xrp/xrp_parse.h"
#include "../src/xrp/xrp_helpers.h"
#include "../src/xrp/fmt.h"

field_name_t field_name;
field_value_t field_value;
parseContext_t parse_context;

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

static void reset_transaction_context(void) {
    explicit_bzero(&parse_context, sizeof(parse_context));
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    // Reset old transaction data that might still remain
    reset_transaction_context();
    parse_context.data = (uint8_t *)Data;
    parse_context.length = Size;

    if (parse_tx(&parse_context) != 0) {
        return 0;
    }

    parseResult_t *transaction = &parse_context.result;
    for (int i = 0; i < transaction->num_fields; ++i) {
        field_t *field = &transaction->fields[i];
        printf("%x\n", field->id);
        update_title(field, &field_name);
        update_value(field, &field_value);
        printf("%s: %s\n", field_name.buf, field_value.buf);
    }

    return 0;
}
