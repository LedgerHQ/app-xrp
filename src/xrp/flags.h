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

#ifndef LEDGER_APP_XRP_FLAGS_H
#define LEDGER_APP_XRP_FLAGS_H

#include <stdbool.h>
#include "fields.h"

// Universal Transaction flags (hidden)
#define TF_FULLY_CANONICAL_SIG 0x80000000u

bool is_flag(const field_t* field);
bool is_flag_hidden(const field_t* field);
void format_flags(field_t* field, field_value_t* dst);

#endif  // LEDGER_APP_XRP_FLAGS_H
