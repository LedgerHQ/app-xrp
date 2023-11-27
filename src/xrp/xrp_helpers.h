/*******************************************************************************
 *   XRP Wallet
 *   (c) 2017 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#pragma once

#include <stdbool.h>

#include "os.h"
#include "cx.h"
#include "fields.h"

#define XRP_PUBKEY_SIZE  33
#define XRP_ADDRESS_SIZE 41

typedef struct {
    uint8_t buf[XRP_PUBKEY_SIZE];
} xrp_pubkey_t;

typedef union {
    xrp_pubkey_t pubkey;
    xrp_account_t account;
} xrp_pubkey_or_account;

typedef struct {
    char buf[XRP_ADDRESS_SIZE];
} xrp_address_t;

cx_err_t xrp_public_key_hash160(xrp_pubkey_t *pubkey, uint8_t *out);

size_t xrp_public_key_to_encoded_base58(xrp_pubkey_t *pubkey,
                                        xrp_account_t *account,
                                        xrp_address_t *out,
                                        uint16_t version);

void xrp_compress_public_key(cx_ecfp_public_key_t *public_key, xrp_pubkey_t *out);

void get_address(cx_ecfp_public_key_t *pubkey, xrp_address_t *address);

int xrp_print_amount(uint64_t amount, char *out, size_t outlen);

bool parse_bip32_path(uint8_t *path,
                      size_t path_length,
                      uint32_t *path_parsed,
                      size_t path_parsed_length);
