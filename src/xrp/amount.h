/*******************************************************************************
 *   XRP Wallet
 *   (c) 2017 Ledger
 *   (c) 2020 Towo Labs
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

#ifndef LEDGER_APP_XRP_AMOUNT_H
#define LEDGER_APP_XRP_AMOUNT_H

#include <stdbool.h>
#include <stddef.h>
#include "fields.h"

void amount_formatter(field_t *field, field_value_t *dst);
void currency_formatter(field_t *field, field_value_t *dst);
void number_formatter(field_t *field, field_value_t *dst);
// A Number in the given asset: XRP from drops, an issued currency with its
// code.
void asset_number_formatter(field_t *field, xrp_currency_t *currency, field_value_t *dst);

// Prints an STNumber, multiplied by 10^shift, into buf. Returns -1, with buf
// empty, if it does not fit.
int print_number(char *buf, size_t size, const uint8_t *number, int32_t shift);
// Whether an STNumber is zero or within the exponent range shown as a plain
// decimal, in its asset's base units or in XRP. No asset holds a value
// outside it: an issued amount's exponent is within -96..80, and an XRP
// amount is a whole number of drops below 2^63.
bool is_number_in_range(const uint8_t *number);
bool is_number_zero(const uint8_t *number);

bool has_non_standard_currency(field_t *field);
bool is_all_zeros(const uint8_t *data, uint8_t length);
bool is_valid_currency(const uint8_t *currency_data);

#define XRP_AMOUNT_LEN      8
#define ISSUED_CURRENCY_LEN 48
#define NUMBER_LEN          12

// Marks an MPT amount: MPT-denominated vaults are not supported yet
#define AMOUNT_MPT_BIT 0x20

#endif  // LEDGER_APP_XRP_AMOUNT_H
