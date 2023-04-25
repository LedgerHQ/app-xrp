/*******************************************************************************
 *   XRP Wallet
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

#ifndef LEDGER_APP_XRP_LOADING_H
#define LEDGER_APP_XRP_LOADING_H

#ifdef HAVE_BAGL
#include "../../transaction/transaction.h"
void execute_async(action_t action_to_load, const char* message);
#endif  // HAVE_BAGL

#endif  // LEDGER_APP_XRP_LOADING_H
