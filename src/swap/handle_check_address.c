#include <string.h>

#include "handle_check_address.h"
#include "os.h"
#include "xrp_helpers.h"
#include "xrp_pub_key.h"

static int os_strcmp(const char* s1, const char* s2) {
    size_t size = strlen(s1) + 1;
    return memcmp(s1, s2, size);
}

int handle_check_address(check_address_parameters_t* params) {
    PRINTF("Params on the address %d\n", (unsigned int) params);
    PRINTF("Address to check %s\n", params->address_to_check);
    PRINTF("Inside handle_check_address\n");

    if (params->address_to_check == NULL) {
        PRINTF("Address to check == NULL\n");
        return 0;
    }

    uint8_t* bip32_path_ptr = params->address_parameters;
    uint8_t bip32_path_length = *(bip32_path_ptr++);
    cx_ecfp_public_key_t public_key;
    int error =
        get_public_key(CX_CURVE_256K1, bip32_path_ptr, bip32_path_length, &public_key, NULL);
    if (error) {
        PRINTF("get_public_key failed\n");
        return 0;
    }

    xrp_address_t address;
    get_address(&public_key, &address);

    if (os_strcmp(address.buf, params->address_to_check) != 0) {
        PRINTF("Addresses don't match\n");
        return 0;
    }

    PRINTF("Addresses match\n");
    return 1;
}
