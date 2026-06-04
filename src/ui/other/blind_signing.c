#ifdef HAVE_NBGL
#include <stdbool.h>
#include "nbgl_use_case.h"
#include "ui.h"
#include "idle_menu.h"
#include "sign_transaction.h"

#ifdef SCREEN_SIZE_WALLET
static void ui_error_blind_signing_choice(bool confirm) {
    if (confirm) {
        display_settings_menu();
    } else {
        display_idle_menu();
    }
}
#else
static void ui_error_blind_signing_ack(void) {
    display_idle_menu();
}
#endif

int ui_error_blind_signing(void) {
#ifdef SCREEN_SIZE_WALLET
    nbgl_useCaseChoice(&ICON_APP_WARNING,
                       "This transaction cannot be clear-signed",
                       "Enable blind signing in the settings to sign this transaction.",
                       "Go to settings",
                       "Reject transaction",
                       ui_error_blind_signing_choice);
#else
    nbgl_useCaseAction(&C_Alert_circle_14px,
                       "Blind signing must\nbe enabled in\nsettings",
                       NULL,
                       ui_error_blind_signing_ack);
#endif
    reject_transaction();
    return 0;
}
#endif
