#include "handle_swap_sign_transaction.h"

#include <string.h>

#include "global.h"
#include "os.h"
#include "swap_lib_calls.h"
#include "swap_utils.h"

// Save the BSS address where we will write the return value when finished
static uint8_t* G_swap_sign_return_value_address;

bool swap_copy_transaction_parameters(create_transaction_parameters_t* params) {
    // first copy parameters to stack, and then to global data.
    // We need this "trick" as the input data position can overlap with btc-app
    // globals
    swapStrings_t stack_data;
    memset(&stack_data, 0, sizeof(stack_data));
    strncpy(stack_data.address, params->destination_address,
            sizeof(stack_data.address) - 1);
    strncpy(stack_data.destination_tag, params->destination_address_extra_id,
            sizeof(stack_data.destination_tag));
    if ((stack_data.address[sizeof(stack_data.address) - 1] != '\0') ||
        (stack_data.destination_tag[sizeof(stack_data.destination_tag) - 1] !=
         '\0')) {
        return false;
    }

    if (!swap_str_to_u64(params->amount, params->amount_length,
                         &stack_data.amount)) {
        return false;
    }

    if (!swap_str_to_u64(params->fee_amount, params->fee_amount_length,
                         &stack_data.fee)) {
        return false;
    }

    // Full reset the global variables
    os_explicit_zero_BSS_segment();
    // Keep the address at which we'll reply the signing status
    G_swap_sign_return_value_address = &params->result;
    // Commit the values read from exchange to the clean global space
    memcpy(&approval_strings.swap, &stack_data, sizeof(stack_data));

    return true;
}

void __attribute__((noreturn)) finalize_exchange_sign_transaction(
    bool is_success) {
    *G_swap_sign_return_value_address = is_success;
    os_lib_end();
}
