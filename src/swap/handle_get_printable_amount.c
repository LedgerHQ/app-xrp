#include <string.h>
#include <stdint.h>

#include "handle_get_printable_amount.h"
#include "../xrp/xrpHelpers.h"
#include "../xrp/format/readers.h"

/* return 0 on error, 1 otherwise */
int handle_get_printable_amount(get_printable_amount_parameters_t* params) {
    params->printable_amount[0] = 0;
    if (params->amount_length > 8) {
        PRINTF("Amount is too big");
        return 0;
    }

    unsigned char buffer[8];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer + sizeof(buffer) - params->amount_length, params->amount, params->amount_length);

    uint64_t amount = readUnsigned64(buffer);
    if (xrp_print_amount(amount, params->printable_amount, sizeof(params->printable_amount)) != 0) {
        PRINTF("xrp_print_amount failed");
        return 0;
    }

    return 1;
}
