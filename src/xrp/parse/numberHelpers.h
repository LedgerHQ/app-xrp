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

#ifndef LEDGER_APP_XRP_NUMBERHELPERS_H
#define LEDGER_APP_XRP_NUMBERHELPERS_H

#include <stdint.h>

#define EXP_MIN      -96
#define EXP_MAX      80
#define MANTISSA_MIN 1000000000000000
#define MANTISSA_MAX 9999999999999999

void parseDecimalNumber(char* dst,
                        uint16_t maxLen,
                        uint8_t sign,
                        int16_t exponent,
                        uint64_t mantissa);
char intToNumberChar(uint64_t value);
void print_uint64_t(char* dst, uint16_t len, uint64_t value);

#endif  // LEDGER_APP_XRP_NUMBERHELPERS_H
