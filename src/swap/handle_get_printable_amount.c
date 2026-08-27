#include <stdint.h>

#include "swap_lib_calls.h"
#include "swap_utils.h"
#include "xrp_helpers.h"

void swap_handle_get_printable_amount(
    get_printable_amount_parameters_t* params) {
    uint64_t amount;

    params->printable_amount[0] = '\x00';

    if (!swap_str_to_u64(params->amount, params->amount_length, &amount)) {
        PRINTF("Amount is too big");
        return;
    }

    if (xrp_print_amount(amount, params->printable_amount,
                         sizeof(params->printable_amount)) != 0) {
        PRINTF("xrp_print_amount failed");
    }
}
