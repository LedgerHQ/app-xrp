#include "handle_check_address.h"
#include "os.h"
#include "string.h"
#include "../xrp/xrpHelpers.h"

#define ZERO(x) os_memset(x, 0, sizeof(x))

void handle_check_address(check_address_parameters_t* params) {
    cx_ecfp_public_key_t public_key;
    PRINTF("Params on the address %d\n", (unsigned int) params);
    PRINTF("Address to check %s\n", params->address_to_check);
    PRINTF("Inside handle_check_address\n");
    params->result = 0;
    if (params->address_to_check == 0) {
        PRINTF("Address to check == 0\n");
        return;
    }

    uint8_t* bip32_path_ptr = params->address_parameters;
    uint8_t bip32PathLength = *(bip32_path_ptr++);
    get_publicKey(CX_CURVE_256K1, bip32_path_ptr, bip32PathLength, &public_key, NULL);

    char address[41];
    get_address(&public_key, address, sizeof(address));

    if ((strlen(address) != strlen(params->address_to_check)) ||
        os_memcmp(address, params->address_to_check, strlen(address)) != 0) {
        // os_memcpy(params->address_to_check, address, strlen(address));
        PRINTF("Addresses doesn't match\n");
        return;
    }
    PRINTF("Addresses  match\n");
    params->result = 1;
}