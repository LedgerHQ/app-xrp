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

#ifndef LEDGER_APP_XRP_GENERAL_H
#define LEDGER_APP_XRP_GENERAL_H

#include "fields.h"

void uint8_formatter(field_t* field, field_value_t* dst);
void uint16_formatter(field_t* field, field_value_t* dst);
void uint32_formatter(field_t* field, field_value_t* dst);
void hash_formatter128(field_t* field, field_value_t* dst);
void hash_formatter256(field_t* field, field_value_t* dst);
void blob_formatter(field_t* field, field_value_t* dst);
void account_formatter(field_t* field, field_value_t* dst);

#endif  // LEDGER_APP_XRP_GENERAL_H
