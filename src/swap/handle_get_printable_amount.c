#include <stdint.h>

#include "handle_get_printable_amount.h"
#include "swap_utils.h"
#include "../xrp/xrp_helpers.h"

/* return 0 on error, 1 otherwise */
int handle_get_printable_amount(get_printable_amount_parameters_t* params) {
    uint64_t amount;

    params->printable_amount[0] = '\x00';

    if (!swap_str_to_u64(params->amount, params->amount_length, &amount)) {
        PRINTF("Amount is too big");
        return 0;
    }

    if (xrp_print_amount(amount, params->printable_amount, sizeof(params->printable_amount)) != 0) {
        PRINTF("xrp_print_amount failed");
        return 0;
    }

    return 1;
}
