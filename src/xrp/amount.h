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
#include "fields.h"

void amount_formatter(field_t* field, field_value_t* dst);
void currency_formatter(field_t* field, field_value_t* dst);

bool has_non_standard_currency(field_t* field);
bool is_all_zeros(const uint8_t *data, uint8_t length);

#define XRP_AMOUNT_LEN      8
#define ISSUED_CURRENCY_LEN 48

#endif  // LEDGER_APP_XRP_AMOUNT_H
