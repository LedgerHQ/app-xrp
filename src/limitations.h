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
// SignerEntries, Memos). This is a coarse per-array cap; the real ceiling is
// MAX_FIELD_COUNT, because each array item expands into a variable number of
// displayed fields: a SignerListSet SignerEntry yields 2 (Account,
// SignerWeight), whereas a multisign Signer yields 3 (Account, Sig.PubKey,
// Txn Sig.). With MAX_FIELD_COUNT = 60 and ~5 base fields, a 2-field array
// (SignerEntries) fits up to (60 - 5) / 2 = 27 items, while a 3-field array
// (Signers) is bounded at 18 by the field budget (append_new_field returns
// NOT_ENOUGH_SPACE beyond that). 27 lets SignerListSet configure the larger
// signer lists enabled by the ExpandedSignerList amendment; transactions whose
// items overflow the 60-field budget are still rejected gracefully.
#define MAX_ARRAY_LEN  27
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
