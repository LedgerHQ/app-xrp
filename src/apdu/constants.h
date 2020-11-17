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

#ifndef LEDGER_APP_XRP_CONSTANTS_H
#define LEDGER_APP_XRP_CONSTANTS_H

#define CLA                       0xE0
#define INS_GET_PUBLIC_KEY        0x02
#define INS_SIGN                  0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define P1_CONFIRM                0x01
#define P1_NON_CONFIRM            0x00
#define P2_NO_CHAINCODE           0x00
#define P2_CHAINCODE              0x01
#define P1_MASK_ORDER             0x01u
#define P1_MASK_MORE              0x80u
#define P2_SECP256K1              0x40u
#define P2_ED25519                0x80u

#define OFFSET_CLA   0
#define OFFSET_INS   1
#define OFFSET_P1    2
#define OFFSET_P2    3
#define OFFSET_LC    4
#define OFFSET_CDATA 5

#endif  // LEDGER_APP_XRP_CONSTANTS_H
