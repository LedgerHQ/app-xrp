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
#include "xrpParse.h"
#include "xrpHelpers.h"

#include "os_io_seproxyhal.h"
#include "string.h"

#include "glyphs.h"

bagl_element_t tmp_element;
unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];


uint32_t set_result_get_publicKey(void);

#define MAX_BIP32_PATH 10

#define CLA 0xE0
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN 0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define P1_CONFIRM 0x01
#define P1_NON_CONFIRM 0x00
#define P2_NO_CHAINCODE 0x00
#define P2_CHAINCODE 0x01
#define P1_FIRST 0x00
#define P1_MORE 0x80
#define P2_SECP256K1 0x40
#define P2_ED25519 0x80

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

#define MAX_RAW_TX 200

static const uint8_t const SIGN_PREFIX[] = { 0x53, 0x54, 0x58, 0x00 };

typedef struct publicKeyContext_t {
    cx_ecfp_public_key_t publicKey;
    uint8_t address[41];
    uint8_t chainCode[32];
    bool getChaincode;
} publicKeyContext_t;

typedef struct transactionContext_t {  
    cx_curve_t curve;  
    uint8_t pathLength;
    uint32_t bip32Path[MAX_BIP32_PATH];    
    uint8_t rawTx[MAX_RAW_TX];
    uint32_t rawTxLength;
} transactionContext_t;

union {
    publicKeyContext_t publicKeyContext;
    transactionContext_t transactionContext;
} tmpCtx;
txContent_t txContent;


volatile uint8_t fidoTransport;
volatile char tag[11]; // uint32t
volatile char tag2[11]; // uint32t
volatile char addressSummary[32];
volatile char fullAddress[43];
volatile char fullAmount[50];
volatile char maxFee[50];
volatile bool dataPresent;

bagl_element_t tmp_element;

unsigned int io_seproxyhal_touch_settings(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e);
void ui_idle(void);

#ifdef HAVE_UX_FLOW
#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
#else // HAVE_UX_FLOW
ux_state_t ux;
#endif // HAVE_UX_FLOW

// display stepped screens
unsigned int ux_step;
unsigned int ux_step_count;

typedef struct internalStorage_t {
    uint8_t initialized;
} internalStorage_t;

WIDE internalStorage_t const N_storage_real;
#define N_storage (*(volatile internalStorage_t *)PIC(&N_storage_real))


const bagl_element_t *ui_menu_item_out_over(const bagl_element_t *e) {
    // the selection rectangle is after the none|touchable
    e = (const bagl_element_t *)(((unsigned int)e) + sizeof(bagl_element_t));
    return e;
}

#define BAGL_FONT_OPEN_SANS_LIGHT_16_22PX_AVG_WIDTH 10
#define BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH 8
#define MAX_CHAR_PER_LINE 25

#define COLOR_BG_1 0xF9F9F9
#define COLOR_APP 0x27a2db
#define COLOR_APP_LIGHT 0x93d1ed

#if defined(TARGET_BLUE)
const bagl_element_t ui_idle_blue[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 68, 320, 413, 0, 0, BAGL_FILL, COLOR_BG_1,
      0x000000, 0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    // erase screen (only under the status bar)
    {{BAGL_RECTANGLE, 0x00, 0, 20, 320, 48, 0, 0, BAGL_FILL, COLOR_APP,
      COLOR_APP, 0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    /// TOP STATUS BAR
    {{BAGL_LABELINE, 0x00, 0, 45, 320, 30, 0, 0, BAGL_FILL, 0xFFFFFF, COLOR_APP,
      BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "XRP",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 264, 19, 56, 44, 0, 0,
      BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT,
      BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER |
          BAGL_FONT_ALIGNMENT_MIDDLE,
      0},
     BAGL_FONT_SYMBOLS_0_DASHBOARD,
     0,
     COLOR_APP,
     0xFFFFFF,
     io_seproxyhal_touch_exit,
     NULL,
     NULL},

    // BADGE_XRP.GIF
    {{BAGL_ICON, 0x00, 135, 178, 50, 50, 0, 0, BAGL_FILL, 0, COLOR_BG_1, 0, 0},
     &C_badge_XRP,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x00, 0, 270, 320, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_LIGHT_16_22PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Open XRP wallet",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x00, 0, 308, 320, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Connect your Ledger Blue and open your",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x00, 0, 331, 320, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "preferred wallet to view your accounts.",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x00, 0, 450, 320, 14, 0, 0, 0, 0x999999, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_8_11PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Validation requests will show automatically.",
     10,
     0,
     COLOR_BG_1,
     NULL,
     NULL,
     NULL},
};

unsigned int ui_idle_blue_button(unsigned int button_mask,
                                 unsigned int button_mask_counter) {
    return 0;
}
#endif // #if TARGET_BLUE

#if defined(TARGET_NANOS) && !defined(HAVE_UX_FLOW)

const ux_menu_entry_t menu_main[];

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
    {menu_main, NULL, 1, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};

const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0, &C_icon_XRP, "Use wallet to", "view accounts", 33, 12},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END};

#endif // #if TARGET_NANOS && !HAVE_UX_FLOW

#if defined(TARGET_BLUE)

const bagl_element_t ui_address_blue[] = {
    {{BAGL_RECTANGLE, 0x00, 0, 68, 320, 413, 0, 0, BAGL_FILL, COLOR_BG_1,
      0x000000, 0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    // erase screen (only under the status bar)
    {{BAGL_RECTANGLE, 0x00, 0, 20, 320, 48, 0, 0, BAGL_FILL, COLOR_APP,
      COLOR_APP, 0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    /// TOP STATUS BAR
    {{BAGL_LABELINE, 0x00, 0, 45, 320, 30, 0, 0, BAGL_FILL, 0xFFFFFF, COLOR_APP,
      BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "CONFIRM ADDRESS",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    //{{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 264,  19,  56,  44, 0, 0,
    //BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT,
    //BAGL_FONT_SYMBOLS_0|BAGL_FONT_ALIGNMENT_CENTER|BAGL_FONT_ALIGNMENT_MIDDLE,
    //0 }, " " /*BAGL_FONT_SYMBOLS_0_DASHBOARD*/, 0, COLOR_APP, 0xFFFFFF,
    //io_seproxyhal_touch_exit, NULL, NULL},

    {{BAGL_LABELINE, 0x00, 30, 106, 320, 30, 0, 0, BAGL_FILL, 0x999999,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     "ADDRESS",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x10, 30, 136, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x11, 30, 159, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 40, 414, 115, 36, 0, 18,
      BAGL_FILL, 0xCCCCCC, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER |
          BAGL_FONT_ALIGNMENT_MIDDLE,
      0},
     "REJECT",
     0,
     0xB7B7B7,
     COLOR_BG_1,
     io_seproxyhal_touch_address_cancel,
     NULL,
     NULL},
    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 165, 414, 115, 36, 0, 18,
      BAGL_FILL, 0x41ccb4, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER |
          BAGL_FONT_ALIGNMENT_MIDDLE,
      0},
     "CONFIRM",
     0,
     0x3ab7a2,
     COLOR_BG_1,
     io_seproxyhal_touch_address_ok,
     NULL,
     NULL},
};

unsigned int ui_address_blue_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int length = strlen(fullAddress);
        if (length >= (element->component.userid & 0xF) * MAX_CHAR_PER_LINE) {
            os_memset(addressSummary, 0, MAX_CHAR_PER_LINE + 1);
            os_memmove(
                addressSummary,
                fullAddress +
                    (element->component.userid & 0xF) * MAX_CHAR_PER_LINE,
                MIN(length -
                        (element->component.userid & 0xF) * MAX_CHAR_PER_LINE,
                    MAX_CHAR_PER_LINE));
            return 1;
        }
        // nothing to draw for this line
        return 0;
    }
    return 1;
}
unsigned int ui_address_blue_button(unsigned int button_mask,
                                    unsigned int button_mask_counter) {
    return 0;
}
#endif // #if defined(TARGET_BLUE)

#if defined(TARGET_NANOS) && !defined(HAVE_UX_FLOW)
const bagl_element_t ui_address_nanos[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF,
      0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CROSS},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CHECK},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    //{{BAGL_ICON                           , 0x01,  31,   9,  14,  14, 0, 0, 0
    //, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_EYE_BADGE  }, NULL, 0, 0, 0,
    //NULL, NULL, NULL },
    {{BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Confirm",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x01, 0, 26, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "address",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x02, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Address",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x02, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
     (char *)fullAddress,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
};

unsigned int ui_address_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid - 1);
        if (display) {
            switch (element->component.userid) {
            case 1:
                UX_CALLBACK_SET_INTERVAL(2000);
                break;
            case 2:
                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
                break;
            }
        }
        return display;
    }
    return 1;
}

unsigned int ui_address_nanos_button(unsigned int button_mask,
                                     unsigned int button_mask_counter);
#endif // #if defined(TARGET_NANOS) && !defined(HAVE_UX_FLOW)

#if defined(TARGET_BLUE)
// reuse addressSummary for each line content
char *ui_details_title;
char *ui_details_content;
typedef void (*callback_t)(void);
callback_t ui_details_back_callback;

const bagl_element_t *
ui_details_blue_back_callback(const bagl_element_t *element) {
    ui_details_back_callback();
    return 0;
}

const bagl_element_t ui_details_blue[] = {
    // erase screen (only under the status bar)
    {{BAGL_RECTANGLE, 0x00, 0, 68, 320, 413, 0, 0, BAGL_FILL, COLOR_BG_1,
      0x000000, 0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_RECTANGLE, 0x00, 0, 20, 320, 48, 0, 0, BAGL_FILL, COLOR_APP,
      COLOR_APP, 0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    /// TOP STATUS BAR
    {{BAGL_LABELINE, 0x01, 0, 45, 320, 30, 0, 0, BAGL_FILL, 0xFFFFFF, COLOR_APP,
      BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 0, 19, 50, 44, 0, 0,
      BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT,
      BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER |
          BAGL_FONT_ALIGNMENT_MIDDLE,
      0},
     BAGL_FONT_SYMBOLS_0_LEFT,
     0,
     COLOR_APP,
     0xFFFFFF,
     ui_details_blue_back_callback,
     NULL,
     NULL},
    //{{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 264,  19,  56,  44, 0, 0,
    //BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT,
    //BAGL_FONT_SYMBOLS_0|BAGL_FONT_ALIGNMENT_CENTER|BAGL_FONT_ALIGNMENT_MIDDLE,
    //0 }, " " /*BAGL_FONT_SYMBOLS_0_DASHBOARD*/, 0, COLOR_APP, 0xFFFFFF,
    //io_seproxyhal_touch_exit, NULL, NULL},

    {{BAGL_LABELINE, 0x00, 30, 106, 320, 30, 0, 0, BAGL_FILL, 0x999999,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     "VALUE",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x10, 30, 136, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x11, 30, 159, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x12, 30, 182, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x13, 30, 205, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x14, 30, 228, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x15, 30, 251, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x16, 30, 274, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x17, 30, 297, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x18, 30, 320, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    //"..." at the end if too much
    {{BAGL_LABELINE, 0x19, 30, 343, 260, 30, 0, 0, BAGL_FILL, 0x000000,
      COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x00, 0, 450, 320, 14, 0, 0, 0, 0x999999, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_8_11PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Review the whole value before continuing.",
     10,
     0,
     COLOR_BG_1,
     NULL,
     NULL,
     NULL},
};

unsigned int ui_details_blue_prepro(const bagl_element_t *element) {
    if (element->component.userid == 1) {
        strcpy(addressSummary, (const char*)PIC(ui_details_title));
    } else if (element->component.userid > 0) {
        unsigned int length = strlen(ui_details_content);
        if (length >= (element->component.userid & 0xF) * MAX_CHAR_PER_LINE) {
            os_memset(addressSummary, 0, MAX_CHAR_PER_LINE + 1);
            os_memmove(
                addressSummary,
                ui_details_content +
                    (element->component.userid & 0xF) * MAX_CHAR_PER_LINE,
                MIN(length -
                        (element->component.userid & 0xF) * MAX_CHAR_PER_LINE,
                    MAX_CHAR_PER_LINE));
            return 1;
        }
        // nothing to draw for this line
        return 0;
    }
    return 1;
}

unsigned int ui_details_blue_button(unsigned int button_mask,
                                    unsigned int button_mask_counter) {
    return 0;
}

void ui_details_init(const char *title, const char *content,
                     callback_t back_callback) {
    ui_details_title = title;
    ui_details_content = content;
    ui_details_back_callback = back_callback;
    UX_DISPLAY(ui_details_blue, ui_details_blue_prepro);
}

void ui_approval_blue_init(void);

bagl_element_callback_t ui_approval_blue_ok;
bagl_element_callback_t ui_approval_blue_cancel;

const bagl_element_t* ui_approval_blue_ok_callback(const bagl_element_t* e) {
  return ui_approval_blue_ok(e);
}

const bagl_element_t* ui_approval_blue_cancel_callback(const bagl_element_t* e) {
  return ui_approval_blue_cancel(e);
}

typedef enum {
  APPROVAL_TRANSACTION_AAF,    
  APPROVAL_TRANSACTION_AASF,    
  APPROVAL_TRANSACTION_AADF,
  APPROVAL_TRANSACTION_AASDF,
  //APPROVAL_MESSAGE, 
} ui_approval_blue_state_t;
ui_approval_blue_state_t G_ui_approval_blue_state;
// pointer to value to be displayed
const char* ui_approval_blue_values[5];
// variable part of the structure
const char* const ui_approval_blue_details_name[][7] = {
  /*APPROVAL_TRANSACTION*/    
  {"AMOUNT",  "ADDRESS", "FEES",  NULL, NULL, "CONFIRM TRANSACTION","Transaction details",},
  {"AMOUNT",  "ADDRESS", "SOURCE TAG", "FEES", NULL, "CONFIRM TRANSACTION","Transaction details",},
  {"AMOUNT",  "ADDRESS", "DESTINATION TAG", "FEES", NULL, "CONFIRM TRANSACTION","Transaction details",},
  {"AMOUNT",  "ADDRESS", "SOURCE TAG", "DESTINATION TAG", "FEES", "CONFIRM TRANSACTION","Transaction details",},
  
  /*APPROVAL_MESSAGE*/ 
  //{"HASH",    NULL,      NULL,      "SIGN MESSAGE",       "Message signature", },  
};

const bagl_element_t* ui_approval_common_show_details(unsigned int detailidx) {
  if (ui_approval_blue_values[detailidx] != NULL && strlen(ui_approval_blue_values[detailidx])*BAGL_FONT_OPEN_SANS_LIGHT_16_22PX_AVG_WIDTH >= 160) {
    // display details screen
    ui_details_init(ui_approval_blue_details_name[G_ui_approval_blue_state][detailidx], ui_approval_blue_values[detailidx], ui_approval_blue_init);
  }
  return NULL;
}

const bagl_element_t* ui_approval_blue_1_details(const bagl_element_t* e) {
    return ui_approval_common_show_details(0);
}

const bagl_element_t* ui_approval_blue_2_details(const bagl_element_t* e) {
    return ui_approval_common_show_details(1);
}

const bagl_element_t* ui_approval_blue_3_details(const bagl_element_t* e) {
    return ui_approval_common_show_details(2);
}

const bagl_element_t* ui_approval_blue_4_details(const bagl_element_t* e) {
    return ui_approval_common_show_details(3);
}

const bagl_element_t* ui_approval_blue_5_details(const bagl_element_t* e) {
    return ui_approval_common_show_details(4);
}

const bagl_element_t ui_approval_blue[] = {
  {{BAGL_RECTANGLE                      , 0x00,   0,  68, 320, 413, 0, 0, BAGL_FILL, COLOR_BG_1, 0x000000, 0                                                                                 , 0   }, NULL, 0, 0, 0, NULL, NULL, NULL },

  // erase screen (only under the status bar)
  {{BAGL_RECTANGLE                      , 0x00,   0,  20, 320,  48, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP, 0                                                      , 0   }, NULL, 0, 0, 0, NULL, NULL, NULL},

  /// TOP STATUS BAR
  {{BAGL_LABELINE                       , 0x60,   0,  45, 320,  30, 0, 0, BAGL_FILL, 0xFFFFFF, COLOR_APP, BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX|BAGL_FONT_ALIGNMENT_CENTER, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL},

  // BADGE_TRANSACTION.GIF
  {{BAGL_ICON                           , 0x40,  30,  98,  50,  50, 0, 0, BAGL_FILL, 0       , COLOR_BG_1, 0                                                                                 , 0  } , &C_badge_transaction, 0, 0, 0, NULL, NULL, NULL },

  {{BAGL_LABELINE                       , 0x50, 100, 117, 320,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL},

  {{BAGL_LABELINE                       , 0x00, 100, 138, 320,  30, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_8_11PX, 0   }, "Check and confirm values", 0, 0, 0, NULL, NULL, NULL},


  {{BAGL_LABELINE                       , 0x70,  30, 196, 100,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, // AMOUNT
  // x-18 when ...
  {{BAGL_LABELINE                       , 0x10, 130, 200, 160,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_LIGHT_16_22PX|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, // fullAmount
  {{BAGL_LABELINE                       , 0x20, 284, 196,   6,  16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
  {{BAGL_NONE   | BAGL_FLAG_TOUCHABLE   , 0x80,   0, 168, 320,  48, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0                                                                                        , 0   }, NULL, 0, 0xEEEEEE, 0x000000, ui_approval_blue_1_details, ui_menu_item_out_over, ui_menu_item_out_over },
  {{BAGL_RECTANGLE                      , 0x20,   0, 168,   5,  48, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0                                                                                    , 0   }, NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL },
  
  {{BAGL_RECTANGLE                      , 0x31,  30, 216, 260,   1, 1, 0, 0        , 0xEEEEEE, COLOR_BG_1, 0                                                                                    , 0   }, NULL, 0, 0, 0, NULL, NULL, NULL },


  {{BAGL_LABELINE                       , 0x71,  30, 245, 100,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, // ADDRESS
  // x-18 when ...
  {{BAGL_LABELINE                       , 0x11, 130, 245, 160,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, // fullAddress
  {{BAGL_LABELINE                       , 0x21, 284, 245,   6,  16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
  {{BAGL_NONE   | BAGL_FLAG_TOUCHABLE   , 0x80,   0, 217, 320,  48, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0                                                                                        , 0   }, NULL, 0, 0xEEEEEE, 0x000000, ui_approval_blue_2_details, ui_menu_item_out_over, ui_menu_item_out_over },
  {{BAGL_RECTANGLE                      , 0x21,   0, 217,   5,  48, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0                                                                                    , 0   }, NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL },  

  {{BAGL_RECTANGLE                      , 0x32,  30, 265, 260,   1, 1, 0, 0        , 0xEEEEEE, COLOR_BG_1, 0                                                                                    , 0   }, NULL, 0, 0, 0, NULL, NULL, NULL },
  

  {{BAGL_LABELINE                       , 0x72,  30, 294, 100,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, // MAX FEES
  // x-18 when ...
  {{BAGL_LABELINE                       , 0x12, 130, 294, 160,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, //maxFee
  {{BAGL_LABELINE                       , 0x22, 284, 294,   6,  16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
  {{BAGL_NONE   | BAGL_FLAG_TOUCHABLE   , 0x80,   0, 266, 320,  48, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0                                                                                        , 0   }, NULL, 0, 0xEEEEEE, 0x000000, ui_approval_blue_3_details, ui_menu_item_out_over, ui_menu_item_out_over },
  {{BAGL_RECTANGLE                      , 0x22,   0, 266,   5,  48, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0                                                                                    , 0   }, NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL },

  {{BAGL_RECTANGLE                      , 0x33,  30, 314, 260,   1, 1, 0, 0        , 0xEEEEEE, COLOR_BG_1, 0                                                                                    , 0   }, NULL, 0, 0, 0, NULL, NULL, NULL },


  {{BAGL_LABELINE                       , 0x73,  30, 343, 100,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, // MAX FEES
  // x-18 when ...
  {{BAGL_LABELINE                       , 0x13, 130, 343, 160,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, //maxFee
  {{BAGL_LABELINE                       , 0x23, 284, 343,   6,  16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
  {{BAGL_NONE   | BAGL_FLAG_TOUCHABLE   , 0x80,   0, 315, 320,  48, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0                                                                                        , 0   }, NULL, 0, 0xEEEEEE, 0x000000, ui_approval_blue_4_details, ui_menu_item_out_over, ui_menu_item_out_over },
  {{BAGL_RECTANGLE                      , 0x23,   0, 315,   5,  48, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0                                                                                    , 0   }, NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL },

  {{BAGL_RECTANGLE                      , 0x34,  30, 363, 260,   1, 1, 0, 0        , 0xEEEEEE, COLOR_BG_1, 0                                                                                    , 0   }, NULL, 0, 0, 0, NULL, NULL, NULL },


  {{BAGL_LABELINE                       , 0x74,  30, 392, 100,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, // MAX FEES
  // x-18 when ...
  {{BAGL_LABELINE                       , 0x14, 130, 392, 160,  30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, NULL, 0, 0, 0, NULL, NULL, NULL}, //maxFee
  {{BAGL_LABELINE                       , 0x24, 284, 392,   6,  16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0|BAGL_FONT_ALIGNMENT_RIGHT, 0   }, BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
  {{BAGL_NONE   | BAGL_FLAG_TOUCHABLE   , 0x80,   0, 364, 320,  48, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0                                                                                        , 0   }, NULL, 0, 0xEEEEEE, 0x000000, ui_approval_blue_5_details, ui_menu_item_out_over, ui_menu_item_out_over },
  {{BAGL_RECTANGLE                      , 0x24,   0, 364,   5,  48, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0                                                                                    , 0   }, NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL },


  {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00,  40, 414, 115,  36, 0,18, BAGL_FILL, 0xCCCCCC, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_11_14PX|BAGL_FONT_ALIGNMENT_CENTER|BAGL_FONT_ALIGNMENT_MIDDLE, 0 }, "REJECT", 0, 0xB7B7B7, COLOR_BG_1, ui_approval_blue_cancel_callback, NULL, NULL},
  {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 165, 414, 115,  36, 0,18, BAGL_FILL, 0x41ccb4, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_11_14PX|BAGL_FONT_ALIGNMENT_CENTER|BAGL_FONT_ALIGNMENT_MIDDLE, 0 }, "CONFIRM", 0, 0x3ab7a2, COLOR_BG_1, ui_approval_blue_ok_callback, NULL, NULL},

};

const bagl_element_t* ui_approval_blue_prepro(const bagl_element_t* element) {
    if (element->component.userid == 0) {
      return 1;
    }
    // none elements are skipped
    if ((element->component.type&(~BAGL_FLAG_TOUCHABLE)) == BAGL_NONE) {
      return 0;
    }
    else {
      switch(element->component.userid&0xF0) {

        // touchable nonetype entries, skip them to avoid latencies (to be fixed in the sdk later on)
        case 0x80:
          return 0;

        // icon
        case 0x40:
          return 1; 
          
        // TITLE
        case 0x60: 
          os_memmove(&tmp_element, element, sizeof(bagl_element_t));
          tmp_element.text = ui_approval_blue_details_name[G_ui_approval_blue_state][5];
          return &tmp_element;
          
        // SUBLINE
        case 0x50: 
          os_memmove(&tmp_element, element, sizeof(bagl_element_t));
          tmp_element.text = ui_approval_blue_details_name[G_ui_approval_blue_state][6];
          return &tmp_element;

        // details label
        case 0x70:
          if (!ui_approval_blue_details_name[G_ui_approval_blue_state][element->component.userid&0xF]) {
            return NULL;
          }
          os_memmove(&tmp_element, element, sizeof(bagl_element_t));
          tmp_element.text = ui_approval_blue_details_name[G_ui_approval_blue_state][element->component.userid&0xF];
          return &tmp_element;

        // detail value
        case 0x10:
          // won't display
          if (!ui_approval_blue_details_name[G_ui_approval_blue_state][element->component.userid&0xF]) {
            return NULL;
          }
          // always display the value
          os_memmove(&tmp_element, element, sizeof(bagl_element_t));
          tmp_element.text = ui_approval_blue_values[(element->component.userid&0xF)];

          // x -= 18 when overflow is detected
          if (strlen(ui_approval_blue_values[(element->component.userid&0xF)])*BAGL_FONT_OPEN_SANS_LIGHT_16_22PX_AVG_WIDTH >= 160) {
            tmp_element.component.x -= 18;
          }
          return &tmp_element;

        // right arrow and left selection rectangle
        case 0x20:
          if (!ui_approval_blue_details_name[G_ui_approval_blue_state][element->component.userid&0xF]) {
            return NULL;
          }
          if (strlen(ui_approval_blue_values[(element->component.userid&0xF)])*BAGL_FONT_OPEN_SANS_LIGHT_16_22PX_AVG_WIDTH < 160) {
            return NULL;
          }

        // horizontal delimiter
        case 0x30:
          return ui_approval_blue_details_name[G_ui_approval_blue_state][element->component.userid&0xF]!=NULL?element:NULL;
      }
    }
    return element;
}
unsigned int ui_approval_blue_button(unsigned int button_mask, unsigned int button_mask_counter) {
  return 0;
}

void ui_approval_blue_init(void) {
  UX_DISPLAY(ui_approval_blue, ui_approval_blue_prepro);
}

void ui_approval_transaction_blue_init(void) {
  unsigned int index=2;
  // wipe all cases
  os_memset(ui_approval_blue_values, 0, sizeof(ui_approval_blue_values));
  ui_approval_blue_ok = (bagl_element_callback_t) io_seproxyhal_touch_tx_ok;
  ui_approval_blue_cancel = (bagl_element_callback_t) io_seproxyhal_touch_tx_cancel;
  ui_approval_blue_values[0] = fullAmount;
  ui_approval_blue_values[1] = fullAddress;
  if (txContent.sourceTagPresent && txContent.destinationTagPresent) {
    G_ui_approval_blue_state = APPROVAL_TRANSACTION_AASDF;  
    SPRINTF(tag, "%u", txContent.sourceTag);
    SPRINTF(tag2, "%u", txContent.destinationTag);
    ui_approval_blue_values[index++] = tag;
    ui_approval_blue_values[index++] = tag2;
  }
  else if (txContent.sourceTagPresent) {
    G_ui_approval_blue_state = APPROVAL_TRANSACTION_AASF;
    SPRINTF(tag, "%u", txContent.sourceTag);
    ui_approval_blue_values[index++] = tag;
  }
  else if (txContent.destinationTagPresent) {
    G_ui_approval_blue_state = APPROVAL_TRANSACTION_AADF;
    SPRINTF(tag2, "%u", txContent.destinationTag);
    ui_approval_blue_values[index++] = tag2;
  }
  else {
    G_ui_approval_blue_state = APPROVAL_TRANSACTION_AAF;
  }
  ui_approval_blue_values[index] = maxFee;
  ui_approval_blue_init();
}

/*
void ui_approval_message_sign_blue_init(void) {
  ui_approval_blue_ok = (bagl_element_callback_t) io_seproxyhal_touch_signMessage_ok;
  ui_approval_blue_cancel = (bagl_element_callback_t) io_seproxyhal_touch_signMessage_cancel;
  G_ui_approval_blue_state = APPROVAL_MESSAGE;
  ui_approval_blue_values[0] = fullAmount;
  ui_approval_blue_values[1] = NULL;
  ui_approval_blue_values[2] = NULL;
  ui_approval_blue_init();
}
*/

#endif // #if defined(TARGET_BLUE)

#if defined(TARGET_NANOS) && !defined(HAVE_UX_FLOW)
const char * const ui_approval_details[][2] = {
    {"Amount", fullAmount},
    {"Address", fullAddress},
    {"Source Tag", tag},
    {"Destination Tag", tag},
    {"Fees", maxFee},
};
const bagl_element_t ui_approval_nanos[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF,
      0, 0},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CROSS},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
      BAGL_GLYPH_ICON_CHECK},
     NULL,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    //{{BAGL_ICON                           , 0x01,  21,   9,  14,  14, 0, 0, 0
    //, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_TRANSACTION_BADGE  }, NULL, 0, 0,
    //0, NULL, NULL, NULL },
    {{BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Confirm",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x01, 0, 26, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "transaction",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x02, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     NULL, //"Amount",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x12, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
     NULL, //(char *)fullAmount,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

     /*
    {{BAGL_LABELINE, 0x03, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Address",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x03, 16, 26, 96, 12, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     (char *)addressSummary,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},

    {{BAGL_LABELINE, 0x04, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Fees",
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
    {{BAGL_LABELINE, 0x04, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
     (char *)maxFee,
     0,
     0,
     0,
     NULL,
     NULL,
     NULL},
     */

};

/*
 ux_step 0: confirm
         1: amount 
         2: address
         3: [sourcetag]
         4: [destinationtag]
         5: fees
*/
unsigned int ui_approval_prepro(const bagl_element_t *element) {
    unsigned int display = 1;
    if (element->component.userid > 0) {
        // display the meta element when at least bigger
        display = (ux_step == element->component.userid - 1) || (element->component.userid >= 0x02 && ux_step >= 1);
        if (display) {
            switch (element->component.userid) {
            case 0x01:
                UX_CALLBACK_SET_INTERVAL(2000);
                break;
            case 0x02:
            case 0x12:
                os_memmove(&tmp_element, element, sizeof(bagl_element_t));
                display = ux_step - 1;
                switch(display) {
                    case 0: // amount
                    case 1: // address
                    display_detail:
                        tmp_element.text = ui_approval_details[display][(element->component.userid)>>4];
                        break;
                    case 2: // source tag
                        if (txContent.sourceTagPresent) {
                            SPRINTF(tag, "%u", txContent.sourceTag);
                            goto display_detail;
                        }
                        if (txContent.destinationTagPresent) {
                            SPRINTF(tag, "%u", txContent.destinationTag);
                            display=3;
                            goto display_detail;
                        }
                        // no break is intentional
                    case 3: // destination tag
                        if (txContent.sourceTagPresent && txContent.destinationTagPresent) {
                            SPRINTF(tag, "%u", txContent.destinationTag);
                            display=3;
                            goto display_detail;
                        }
                        // no break is intentional
                    case 4: // fees
                        display=4;
                        goto display_detail;
                }

                UX_CALLBACK_SET_INTERVAL(MAX(
                    3000, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
                return &tmp_element;
            }
        }
    }
    return display;
}

unsigned int ui_approval_nanos_button(unsigned int button_mask,
                                      unsigned int button_mask_counter);

#endif // #if defined(TARGET_NANOS) !defined(HAVE_UX_FLOW)

#if defined(HAVE_UX_FLOW)
//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_idle_flow_1_step, 
    pnn, //pnn, 
    {
      &C_icon_XRP,
      "Application",
      "is ready",
    });
UX_STEP_NOCB(
    ux_idle_flow_2_step, 
    bn, 
    {
      "Version",
      APPVERSION,
    });
UX_STEP_VALID(
    ux_idle_flow_3_step,
    pb,
    os_sched_exit(-1),
    {
      &C_icon_dashboard_x,
      "Quit",
    });


UX_DEF(ux_idle_flow,
    &ux_idle_flow_1_step,
    &ux_idle_flow_2_step,
    &ux_idle_flow_3_step
);

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_display_address_flow_1_step, 
    pnn, 
    {
      &C_icon_eye,
      "Verify",
      "address",
    });
UX_STEP_NOCB(
    ux_display_address_flow_4_step, 
    bnnn_paging, 
    {
      .title = "Address",
      .text = fullAddress,
    });
UX_STEP_VALID(
    ux_display_address_flow_5_step, 
    pb, 
    io_seproxyhal_touch_address_ok(NULL),
    {
      &C_icon_validate_14,
      "Approve",
    });
UX_STEP_VALID(
    ux_display_address_flow_6_step, 
    pb, 
    io_seproxyhal_touch_address_cancel(NULL),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_DEF(ux_display_address_flow,
    &ux_display_address_flow_1_step,
    &ux_display_address_flow_4_step,
    &ux_display_address_flow_5_step,
    &ux_display_address_flow_6_step
);

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(ux_approval_1_step, 
    pnn, 
    {
      &C_icon_eye,
      "Review",
      "transaction",
    });
UX_STEP_NOCB(
    ux_approval_2_step, 
    bnnn_paging, 
    {
      .title = "Amount",
      .text = fullAmount,
    });
UX_STEP_NOCB(
    ux_approval_3_step, 
    bnnn_paging, 
    {
      .title = "Address",
      .text = fullAddress,
    });
UX_STEP_NOCB(
    ux_approval_4_step, 
    bnnn_paging, 
    {
      .title = "Source Tag",
      .text = tag,
    });
UX_STEP_NOCB(
    ux_approval_5_step, 
    bnnn_paging, 
    {
      .title = "Destination Tag",
      .text = tag2,
    });
UX_STEP_NOCB(
    ux_approval_6_step, 
    bnnn_paging, 
    {
      .title = "Fees",
      .text = maxFee,
    });
UX_STEP_VALID(
    ux_approval_7_step, 
    pbb, 
    io_seproxyhal_touch_tx_ok(NULL),
    {
      &C_icon_validate_14,
      "Accept",
      "and send",
    });
UX_STEP_VALID(
    ux_approval_8_step, 
    pb, 
    io_seproxyhal_touch_tx_cancel(NULL),
    {
      &C_icon_crossmark,
      "Reject",
    });
// confirm_full: confirm transaction / Amount: fullAmount / Address: fullAddress / Fees: feesAmount

const ux_flow_step_t * ux_approval_flow[9];

#endif // #if defined(HAVE_UX_FLOW)


void ui_idle(void) {
#if defined(TARGET_BLUE)
    UX_DISPLAY(ui_idle_blue, NULL);
#elif defined(HAVE_UX_FLOW)
    // reserve a display stack slot if none yet
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
#elif defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_main, NULL);
#endif // #if TARGET_ID
}

unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e) {
    // Go back to the dashboard
    os_sched_exit(0);
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e) {
    uint32_t tx = set_result_get_publicKey();
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

#if defined(TARGET_NANOS)
unsigned int ui_address_nanos_button(unsigned int button_mask,
                                     unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT: // CANCEL
        io_seproxyhal_touch_address_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: { // OK
        io_seproxyhal_touch_address_ok(NULL);
        break;
    }
    }
    return 0;
}
#endif // #if defined(TARGET_NANOS)

unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e) {
    uint8_t privateKeyData[64];
    cx_ecfp_private_key_t privateKey;
    uint32_t tx = 0;

    io_seproxyhal_io_heartbeat();
    os_perso_derive_node_bip32(
        tmpCtx.transactionContext.curve, tmpCtx.transactionContext.bip32Path,
        tmpCtx.transactionContext.pathLength, privateKeyData, NULL);    
    cx_ecfp_init_private_key(tmpCtx.transactionContext.curve, privateKeyData, 32, &privateKey);
    os_memset(privateKeyData, 0, sizeof(privateKeyData));
    io_seproxyhal_io_heartbeat();
    if (tmpCtx.transactionContext.curve == CX_CURVE_256K1) {
        cx_hash_sha512(tmpCtx.transactionContext.rawTx, tmpCtx.transactionContext.rawTxLength, privateKeyData, 64);
        PRINTF("Hash to sign:\n%.*H\n", 32, privateKeyData);
        io_seproxyhal_io_heartbeat();
        tx = cx_ecdsa_sign(&privateKey, CX_RND_RFC6979 | CX_LAST, CX_SHA256,
                      privateKeyData,
                      32, G_io_apdu_buffer, sizeof(G_io_apdu_buffer), NULL);
        G_io_apdu_buffer[0] = 0x30;
    }
    else {
        tx = cx_eddsa_sign(&privateKey, CX_LAST, CX_SHA512, tmpCtx.transactionContext.rawTx,
            tmpCtx.transactionContext.rawTxLength, NULL, 0, G_io_apdu_buffer, sizeof(G_io_apdu_buffer), NULL);
    }

    os_memset(&privateKey, 0, sizeof(privateKey));
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();

    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

#if defined(TARGET_NANOS)

unsigned int ui_approval_nanos_button(unsigned int button_mask,
                                      unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        io_seproxyhal_touch_tx_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
        io_seproxyhal_touch_tx_ok(NULL);
        break;
    }
    }
    return 0;
}

#endif // #if defined(TARGET_NANOS)

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

uint32_t set_result_get_publicKey() {
    uint32_t tx = 0;
    uint32_t addressLength = strlen(tmpCtx.publicKeyContext.address);
    G_io_apdu_buffer[tx++] = 33;
    xrp_compress_public_key(&tmpCtx.publicKeyContext.publicKey, G_io_apdu_buffer + tx, 33);
    tx += 33;
    G_io_apdu_buffer[tx++] = addressLength;
    os_memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.address, addressLength);
    tx += addressLength;
    if (tmpCtx.publicKeyContext.getChaincode) {
        os_memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.chainCode,
                   32);
        tx += 32;
    }
    return tx;
}

void handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer,
                        uint16_t dataLength, volatile unsigned int *flags,
                        volatile unsigned int *tx) {
    UNUSED(dataLength);
    uint8_t privateKeyData[33];
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint32_t i;
    uint8_t bip32PathLength = *(dataBuffer++);
    cx_ecfp_private_key_t privateKey;
    uint8_t p2Chain = p2 & 0x3F;
    cx_curve_t curve;
    uint8_t addressLength;

    if ((bip32PathLength < 0x01) || (bip32PathLength > MAX_BIP32_PATH)) {
        PRINTF("Invalid path\n");
        THROW(0x6a80);
    }
    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        THROW(0x6B00);
    }
    if ((p2Chain != P2_CHAINCODE) && (p2Chain != P2_NO_CHAINCODE)) {
        THROW(0x6B00);
    }
    if (((p2 & P2_SECP256K1) == 0) && ((p2 & P2_ED25519) == 0)) {
        THROW(0x6B00);
    }
    if (((p2 & P2_SECP256K1) != 0) && ((p2 & P2_ED25519) != 0)) {
        THROW(0x6B00);
    }    
    curve = (((p2 & P2_ED25519) != 0) ? CX_CURVE_Ed25519 : CX_CURVE_256K1);

    for (i = 0; i < bip32PathLength; i++) {
        bip32Path[i] = (dataBuffer[0] << 24) | (dataBuffer[1] << 16) |
                       (dataBuffer[2] << 8) | (dataBuffer[3]);
        dataBuffer += 4;
    }
    tmpCtx.publicKeyContext.getChaincode = (p2Chain == P2_CHAINCODE);
    io_seproxyhal_io_heartbeat();
    os_perso_derive_node_bip32(curve, bip32Path, bip32PathLength,
                               privateKeyData,
                               (tmpCtx.publicKeyContext.getChaincode
                                    ? tmpCtx.publicKeyContext.chainCode
                                    : NULL));
    cx_ecfp_init_private_key(curve, privateKeyData, 32, &privateKey);
    io_seproxyhal_io_heartbeat();
    cx_ecfp_generate_pair(curve, &tmpCtx.publicKeyContext.publicKey,
                          &privateKey, 1);
    os_memset(&privateKey, 0, sizeof(privateKey));
    os_memset(privateKeyData, 0, sizeof(privateKeyData));
    io_seproxyhal_io_heartbeat();
    xrp_compress_public_key(&tmpCtx.publicKeyContext.publicKey, privateKeyData, 33);
    io_seproxyhal_io_heartbeat();
    addressLength = xrp_public_key_to_encoded_base58(privateKeyData, 33, tmpCtx.publicKeyContext.address, sizeof(tmpCtx.publicKeyContext.address), 0, 0);
    tmpCtx.publicKeyContext.address[addressLength] = '\0';
    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_publicKey();
        THROW(0x9000);
    } 
    else {
        os_memset(fullAddress, 0, sizeof(fullAddress));
        strcpy(fullAddress, tmpCtx.publicKeyContext.address);

        // prepare for a UI based reply
#if defined(TARGET_BLUE)
        UX_DISPLAY(ui_address_blue, ui_address_blue_prepro);
#elif defined(HAVE_UX_FLOW)
        ux_flow_init(0, ux_display_address_flow, NULL);
#elif defined(TARGET_NANOS)
        ux_step = 0;
        ux_step_count = 2;
        UX_DISPLAY(ui_address_nanos, ui_address_prepro);
#endif // #if TARGET_ID

        *flags |= IO_ASYNCH_REPLY;
    }
}

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer,
                uint16_t dataLength, volatile unsigned int *flags,
                volatile unsigned int *tx) {
    UNUSED(tx);
    uint8_t addressLength;
    uint32_t i;
    if (p1 == P1_FIRST) {
        tmpCtx.transactionContext.pathLength = workBuffer[0];
        if ((tmpCtx.transactionContext.pathLength < 0x01) ||
            (tmpCtx.transactionContext.pathLength > MAX_BIP32_PATH)) {
            PRINTF("Invalid path\n");
            THROW(0x6a81);
        }
        workBuffer++;
        dataLength--;
        for (i = 0; i < tmpCtx.transactionContext.pathLength; i++) {
            tmpCtx.transactionContext.bip32Path[i] =
                (workBuffer[0] << 24) | (workBuffer[1] << 16) |
                (workBuffer[2] << 8) | (workBuffer[3]);
            workBuffer += 4;
            dataLength -= 4;
        }
        if (((p2 & P2_SECP256K1) == 0) && ((p2 & P2_ED25519) == 0)) {
            THROW(0x6B00);
        }
        if (((p2 & P2_SECP256K1) != 0) && ((p2 & P2_ED25519) != 0)) {
            THROW(0x6B00);
        }    
        tmpCtx.transactionContext.curve = (((p2 & P2_ED25519) != 0) ? CX_CURVE_Ed25519 : CX_CURVE_256K1);
    }
    else {
        THROW(0x6B00);
    }
    if (parseTx(workBuffer, dataLength, &txContent) != USTREAM_FINISHED) {
        THROW(0x6A82);
    }
    xrp_print_amount(txContent.amount, fullAmount, sizeof(fullAmount));
    xrp_print_amount(txContent.fees, maxFee, sizeof(fullAmount));
    addressLength = xrp_public_key_to_encoded_base58(txContent.destination, 20, tmpCtx.publicKeyContext.address, sizeof(tmpCtx.publicKeyContext.address), 0, 1);
    tmpCtx.publicKeyContext.address[addressLength] = '\0';

    strcpy(fullAddress, tmpCtx.publicKeyContext.address);

    os_memmove(tmpCtx.transactionContext.rawTx, SIGN_PREFIX, sizeof(SIGN_PREFIX));
    os_memmove(tmpCtx.transactionContext.rawTx + sizeof(SIGN_PREFIX), workBuffer, dataLength);
    tmpCtx.transactionContext.rawTxLength = dataLength + sizeof(SIGN_PREFIX);


#if defined(TARGET_BLUE)
    ux_step_count = 0;
    ui_approval_transaction_blue_init();
#elif defined(TARGET_NANOS) && !defined(HAVE_UX_FLOW)
    ux_step = 0;
    // "confirm", amount, address, [sourcetag], [destinationtag], fees
    ux_step_count = 4;
    if (txContent.sourceTagPresent) {
        ux_step_count++;
    }
    if (txContent.destinationTagPresent) {
        ux_step_count++;
    }
    UX_DISPLAY(ui_approval_nanos, ui_approval_prepro);
#endif // #if TARGET
#ifdef HAVE_UX_FLOW
    uint8_t step = 0;
    ux_approval_flow[step++] = &ux_approval_1_step;
    ux_approval_flow[step++] = &ux_approval_2_step;
    ux_approval_flow[step++] = &ux_approval_3_step;
    if(txContent.sourceTagPresent && txContent.destinationTagPresent) {
        SPRINTF(tag, "%u", txContent.sourceTag);
        SPRINTF(tag2, "%u", txContent.destinationTag);
        ux_approval_flow[step++] = &ux_approval_4_step;
        ux_approval_flow[step++] = &ux_approval_5_step;
    }
    else if(!txContent.sourceTagPresent && txContent.destinationTagPresent) {
        SPRINTF(tag2, "%u", txContent.destinationTag);
        ux_approval_flow[step++] = &ux_approval_5_step;
    }
    else if(txContent.sourceTagPresent && !txContent.destinationTagPresent) {
        SPRINTF(tag, "%u", txContent.sourceTag);
        ux_approval_flow[step++] = &ux_approval_4_step;
    }
    ux_approval_flow[step++] = &ux_approval_6_step;
    ux_approval_flow[step++] = &ux_approval_7_step;
    ux_approval_flow[step++] = &ux_approval_8_step;
    ux_approval_flow[step++] = FLOW_END_STEP;

    ux_flow_init(0, ux_approval_flow, NULL);
#endif // HAVE_UX_FLOW

    *flags |= IO_ASYNCH_REPLY;
}

void handleGetAppConfiguration(uint8_t p1, uint8_t p2, uint8_t *workBuffer,
                               uint16_t dataLength,
                               volatile unsigned int *flags,
                               volatile unsigned int *tx) {
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(workBuffer);
    UNUSED(dataLength);
    UNUSED(flags);
    G_io_apdu_buffer[0] = 0x00;
    G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
    G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
    G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
    *tx = 4;
    THROW(0x9000);
}


void handleApdu(volatile unsigned int *flags, volatile unsigned int *tx) {
    unsigned short sw = 0;

    BEGIN_TRY {
        TRY {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(0x6E00);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
            case INS_GET_PUBLIC_KEY:
                handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1],
                                   G_io_apdu_buffer[OFFSET_P2],
                                   G_io_apdu_buffer + OFFSET_CDATA,
                                   G_io_apdu_buffer[OFFSET_LC], flags, tx);
                break;

            case INS_SIGN:
                handleSign(G_io_apdu_buffer[OFFSET_P1],
                           G_io_apdu_buffer[OFFSET_P2],
                           G_io_apdu_buffer + OFFSET_CDATA,
                           G_io_apdu_buffer[OFFSET_LC], flags, tx);
                break;

            case INS_GET_APP_CONFIGURATION:
                handleGetAppConfiguration(
                    G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2],
                    G_io_apdu_buffer + OFFSET_CDATA,
                    G_io_apdu_buffer[OFFSET_LC], flags, tx);
                break;

            default:
                THROW(0x6D00);
                break;
            }
        }
        CATCH_OTHER(e) {
            switch (e & 0xF000) {
            case 0x6000:
                // Wipe the transaction context and report the exception
                sw = e;
                os_memset(&txContent, 0, sizeof(txContent));
                break;
            case 0x9000:
                // All is well
                sw = e;
                break;
            default:
                // Internal error
                sw = 0x6800 | (e & 0x7FF);
                break;
            }
            // Unexpected exception => report
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY {
        }
    }
    END_TRY;
}

void sample_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);

                handleApdu(&flags, &tx);
            }
            CATCH(EXCEPTION_IO_RESET){
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    os_memset(&txContent, 0, sizeof(txContent));
                    break;
                case 0x9000:
                    // All is well
                    sw = e;
                    break;
                default:
                    // Internal error
                    sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    // return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
        UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_STATUS_EVENT:
        if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
            !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
              SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
            THROW(EXCEPTION_IO_RESET);
        }
    // no break is intentional
    default:
        UX_DEFAULT_EVENT();
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({});
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
            if (UX_ALLOWED) {
#if 0
                if (skipWarning && (ux_step == 0)) {
                    ux_step++;
                }
#endif

                if (ux_step_count) {
                    // prepare next screen
                    ux_step = (ux_step + 1) % ux_step_count;
                    // redisplay screen
                    UX_REDISPLAY();
                }
            }
        });
        break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

void app_exit(void) {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

    for (;;) {
	    os_memset(&txContent, 0, sizeof(txContent));
	
        UX_INIT();
        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

#ifdef TARGET_NANOX
                // grab the current plane mode setting
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif // TARGET_NANOX

                /*
                if (N_storage.initialized != 0x01) {
                    internalStorage_t storage;
                    storage.initialized = 0x01;
                    nvm_write(&N_storage, (void *)&storage,
                              sizeof(internalStorage_t));
                }
                */
                USB_power(0);
                USB_power(1);

                ui_idle();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif // HAVE_BLE

    #if defined(TARGET_BLUE)
                // setup the status bar colors (remembered after wards, even more if
                // another app does not resetup after app switch)
                UX_SET_STATUS_BAR_COLOR(0xFFFFFF, COLOR_APP);
    #endif // #if defined(TARGET_BLUE)

                sample_main();
            }
                CATCH(EXCEPTION_IO_RESET) {
                    // reset IO and UX
                    continue;
                }
                CATCH_ALL {
                    break;
                }
            FINALLY {
            }
        }
        END_TRY;
    }
    app_exit();

    return 0;
}
