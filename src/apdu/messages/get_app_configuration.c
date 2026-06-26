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

#include "get_app_configuration.h"

#include <os.h>
#include "io.h"

int handle_get_app_configuration(void) {
    uint8_t buf[4];
    uint32_t tx = 0;
    buf[0] = 0x00;
    buf[1] = MAJOR_VERSION;
    buf[2] = MINOR_VERSION;
    buf[3] = PATCH_VERSION;
    tx = 4;
    return io_send_response_pointer(buf, tx, 0x9000);
}
