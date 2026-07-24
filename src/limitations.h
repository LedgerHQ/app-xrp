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
// Maximum number of entries allowed in an array field (e.g. Signers,
// SignerEntries, Memos). A single array item can expand into as many as three
// displayed fields (a multisign Signer yields Sig.PubKey, Txn Sig. and
// Account), so the effective ceiling is governed by MAX_FIELD_COUNT. On the
// largest devices (MAX_FIELD_COUNT = 60) an 18-item array uses 18 * 3 = 54
// fields which, together with the base transaction fields, still fits the
// budget; 19 items would overflow it. 18 is therefore the highest value that
// lets a fully populated multisigned transaction render on the latest models.
#define MAX_ARRAY_LEN  18
#define MAX_PATH_COUNT 6
#define MAX_STEP_COUNT 8

#define MAX_FIELD_COUNT 60
#define MAX_RAW_TX      10000
#ifdef TARGET_NANOS
#define MAX_FIELD_LEN          128
#define DISPLAY_SEGMENTED_ADDR true
#else
#define MAX_FIELD_LEN          1024
#define DISPLAY_SEGMENTED_ADDR false
#endif

#endif  // LEDGER_APP_XRP_LIMITATIONS_H
