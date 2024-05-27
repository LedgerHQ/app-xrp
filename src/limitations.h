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

#ifndef LEDGER_APP_XRP_LIMITATIONS_H
#define LEDGER_APP_XRP_LIMITATIONS_H

// Needed to resolve target macros
#include "bolos_target.h"

// Hardware independent limits
#define MAX_BIP32_PATH     10
#define MAX_ENC_INPUT_SIZE 26
#define MAX_FIELDNAME_LEN  50
#define MAX_ARRAY_LEN      8
#define MAX_PATH_COUNT     6
#define MAX_STEP_COUNT     8

// Hardware dependent limits
//   Ledger Nano X has 30K RAM
//   Ledger Nano S has 4K RAM
#if defined(TARGET_NANOS)

#define MAX_FIELD_COUNT        24
#define MAX_FIELD_LEN          128
#define MAX_RAW_TX             800
#define DISPLAY_SEGMENTED_ADDR true

#else

#define MAX_FIELD_COUNT        60
#define MAX_FIELD_LEN          1024
#define MAX_RAW_TX             10000
#define DISPLAY_SEGMENTED_ADDR false

#endif

#endif  // LEDGER_APP_XRP_LIMITATIONS_H
