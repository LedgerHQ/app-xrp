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

#include "ascii_strings.h"

bool is_purely_ascii(const uint8_t *data, uint16_t length, bool allow_suffix) {
    bool tracking_suffix = false;

    for (uint16_t i = 0; i < length; ++i) {
        if (tracking_suffix && data[i] != 0) {
            // The suffix can only contain null bytes
            return false;
        }

        if (data[i] == 0 && i > 0 && allow_suffix) {
            tracking_suffix = true;
            continue;
        }

        if (data[i] < 32 || data[i] > 126) {
            return false;
        }
    }

    return true;
}
