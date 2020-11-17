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

#ifndef LEDGER_APP_XRP_UX_H
#define LEDGER_APP_XRP_UX_H

#if defined(TARGET_NANOX)
#define DEV_SCREEN_H 64
#elif defined(TARGET_NANOS)
#define DEV_SCREEN_H 32
#endif

#define UI_BACKGROUND() \
    { {BAGL_RECTANGLE, 0, 0, 0, 128, DEV_SCREEN_H, 0, 0, BAGL_FILL, 0, 0xFFFFFF, 0, 0}, NULL }
#define UI_DUMMY(userid) \
    { {BAGL_RECTANGLE, userid, 0, 0, 0, 0, 0, 0, BAGL_FILL, 0, 0xFFFFFF, 0, 0}, NULL }
#define UI_SINGLE_TEXT(text)                                              \
    {                                                                     \
        {BAGL_LABELINE,                                                   \
         0,                                                               \
         0,                                                               \
         DEV_SCREEN_H / 2 + 2,                                            \
         128,                                                             \
         12,                                                              \
         0,                                                               \
         0,                                                               \
         0,                                                               \
         0xFFFFFF,                                                        \
         0,                                                               \
         BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, \
         0},                                                              \
            (char *) (text)                                               \
    }

#endif  // LEDGER_APP_XRP_UX_H
