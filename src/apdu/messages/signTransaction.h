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

#ifndef LEDGER_APP_XRP_SIGNTRANSACTION_H
#define LEDGER_APP_XRP_SIGNTRANSACTION_H

#include <stdint.h>
#include "../../xrp/parse/xrpParse.h"

extern parseContext_t parseContext;

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer,
                uint8_t dataLength, volatile unsigned int *flags);

#endif //LEDGER_APP_XRP_SIGNTRANSACTION_H
