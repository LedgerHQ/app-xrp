#include "handle_get_printable_amount.h"
#include "../xrp/xrpHelpers.h"
#include "../xrp/format/readers.h"
#include "string.h"
#include <stdint.h>

void handle_get_printable_amount(get_printable_amount_parameters_t* params) {
    params->result = 0;
    params->printable_amount[0] = 0;
    if (params->amount_length > 8) {
        PRINTF("Amount is too big");
        return;
    }

    unsigned char buffer[8];
    os_memset(buffer, 0, sizeof(buffer));
    os_memcpy(buffer + sizeof(buffer) - params->amount_length,
              params->amount,
              params->amount_length);

    uint64_t amount = readUnsigned64(buffer);
    if (xrp_print_amount(amount, params->printable_amount, sizeof(params->printable_amount)) != 0) {
        PRINTF("xrp_print_amount failed");
        return;
    }

    params->result = 1;
}
