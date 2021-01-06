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

#include <string.h>
#include "global.h"
#include "messages/sign_transaction.h"

tmpCtx_t tmp_ctx;
signState_e sign_state;
approvalStrings_t approval_strings;
bool called_from_swap;

void reset_transaction_context() {
    explicit_bzero(&parse_context, sizeof(parseContext_t));
    explicit_bzero(&tmp_ctx, sizeof(tmp_ctx));

    sign_state = IDLE;

    if (!called_from_swap) {
        explicit_bzero(&approval_strings, sizeof(approval_strings));
    }
}
