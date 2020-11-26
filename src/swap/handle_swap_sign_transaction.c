#include "handle_swap_sign_transaction.h"
#include "ux.h"
#include "../apdu/global.h"

void copy_transaction_parameters(create_transaction_parameters_t* sign_transaction_params) {
    // first copy parameters to stack, and then to global data.
    // We need this "trick" as the input data position can overlap with btc-app globals
    swapStrings_t stack_data;
    memset(&stack_data, 0, sizeof(stack_data));
    strncpy(stack_data.address,
            sign_transaction_params->destination_address,
            sizeof(stack_data.address) - 1);
    strncpy(stack_data.destination_tag,
            sign_transaction_params->destination_address_extra_id,
            sizeof(stack_data.destination_tag));
    if ((stack_data.address[sizeof(stack_data.address) - 1] != '\0') ||
        (stack_data.destination_tag[sizeof(stack_data.destination_tag) - 1] != '\0') ||
        (sign_transaction_params->amount_length > 8) ||
        (sign_transaction_params->fee_amount_length > 8)) {
        os_lib_end();
    }
    // store amount as big endian in 8 bytes, so the passed data should be alligned to right
    // input {0xEE, 0x00, 0xFF} should be stored like {0x00, 0x00, 0x00, 0x00, 0x00, 0xEE, 0x00,
    // 0xFF}
    memcpy(stack_data.amount + 8 - sign_transaction_params->amount_length,
           sign_transaction_params->amount,
           sign_transaction_params->amount_length);
    memcpy(stack_data.fees + 8 - sign_transaction_params->fee_amount_length,
           sign_transaction_params->fee_amount,
           sign_transaction_params->fee_amount_length);
    memcpy(&approvalStrings.swap, &stack_data, sizeof(stack_data));
}

void handle_swap_sign_transaction(create_transaction_parameters_t* sign_transaction_params) {
    copy_transaction_parameters(sign_transaction_params);
    called_from_swap = true;
    resetTransactionContext();
    io_seproxyhal_init();
    UX_INIT();
    USB_power(0);
    USB_power(1);
    // ui_idle();
    PRINTF("USB power ON/OFF\n");
#ifdef TARGET_NANOX
    // grab the current plane mode setting
    G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif  // TARGET_NANOX
#ifdef HAVE_BLE
    BLE_power(0, NULL);
    BLE_power(1, "Nano X");
#endif  // HAVE_BLE
    app_main();
}