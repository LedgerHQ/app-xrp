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

#ifndef LEDGER_APP_XRP_UI_H
#define LEDGER_APP_XRP_UI_H

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define ICON_APP_HOME    C_icon_XRP
#define ICON_APP_WARNING C_icon_warning
#elif defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_HOME    C_icon_XRP_64px
#define ICON_APP_WARNING LARGE_WARNING_ICON
#elif defined(TARGET_APEX_P)
#define ICON_APP_HOME    C_icon_XRP_48px
#define ICON_APP_WARNING LARGE_WARNING_ICON
#endif

#endif  // LEDGER_APP_XRP_UI_H
