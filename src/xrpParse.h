/*******************************************************************************
*   XRP Wallet
*   (c) 2017 Ledger
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

#include "os.h"
#include "cx.h"
#include <stdbool.h>

#define MAX_BIP32_PATH 10

typedef enum parserStatus_e {
    USTREAM_PROCESSING,
    USTREAM_FINISHED,
    USTREAM_FAULT
} parserStatus_e;

typedef struct txContent_t {
    uint64_t amount;
    uint64_t fees;
    uint8_t account[20];
    uint8_t destination[20];    
    uint32_t sourceTag;
    uint8_t sourceTagPresent;
    uint32_t destinationTag;
    uint8_t destinationTagPresent;
} txContent_t;

parserStatus_e parseTx(uint8_t *data, uint32_t length, txContent_t *context);

