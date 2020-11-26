#include "handle_get_printable_amount.h"
#include "../xrp/xrpHelpers.h"
#include "../xrp/format/readers.h"
#include "string.h"
#include <stdint.h>

void handle_get_printable_amount(get_printable_amount_parameters_t* params) {
    params->printable_amount[0] = 0;
    if (params->amount_length > 8) {
        PRINTF("Amount is too big");
        return;
    }
    unsigned char amount_buffer[8];
    os_memset(amount_buffer, 0, 8);
    os_memcpy(amount_buffer + (8 - params->amount_length), params->amount, params->amount_length);
    uint64_t amount = readUnsigned64(amount_buffer);
    xrp_print_amount(amount, params->printable_amount, sizeof(params->printable_amount));
}