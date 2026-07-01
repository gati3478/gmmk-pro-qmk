#include QMK_KEYBOARD_H

enum layers { _BASE, _FN };

enum custom_keycodes {
    OS_SHOT_CLIP = SAFE_RANGE, // screenshot region -> clipboard
    OS_SHOT_FILE,              // screenshot region -> file
    OS_MCTL,                   // mission control / overview
    OS_SPOT,                   // spotlight / search
    OS_WORD_L, OS_WORD_R,      // word left / right
    OS_DOC_UP, OS_DOC_DN,      // document top / bottom
    KNOB_MODE,                 // (Task 3) cycle knob mode
    SIDE_TOGG,                 // (Task 6) toggle side LEDs
};

// Fixed combos the user binds once in macOS (Dictation / Do-Not-Disturb).
#define MAC_DICT LCTL(LALT(LGUI(KC_D)))
#define MAC_DND  LCTL(LALT(LGUI(KC_N)))

enum knob_modes { KNOB_SCROLL, KNOB_UNDO };
static uint8_t knob_mode = KNOB_SCROLL;

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_BASE] = LAYOUT(
    KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  OS_SHOT_CLIP, KC_MPLY,
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC,      KC_DEL,
    KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,      KC_END,
    LT(_FN,KC_CAPS), KC_A, KC_S, KC_D,  KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,       KC_PGUP,
    KC_LSFT,          KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, KC_UP,        KC_PGDN,
    KC_LCTL, KC_LALT, KC_LGUI,                   KC_SPC,                             KC_RCTL, KC_RALT, MO(_FN), KC_LEFT, KC_DOWN, KC_RGHT
  ),
  [_FN] = LAYOUT(
    _______, KC_BRID, KC_BRIU, OS_MCTL, OS_SPOT, MAC_DICT, MAC_DND, KC_MPRV, KC_MPLY, KC_MNXT, KC_MUTE, KC_VOLD, KC_VOLU, OS_SHOT_FILE, KNOB_MODE,
    RM_TOGG, RM_NEXT, RM_PREV, RM_HUEU, RM_SATU, RM_VALU, RM_VALD, RM_SPDU, RM_SPDD, SIDE_TOGG,_______, _______, _______, _______,      _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,      KC_HOME,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,      _______,
    _______, _______, _______, _______, _______, QK_BOOT, NK_TOGG, _______, _______, _______, _______, _______, OS_DOC_UP,             _______,
    _______, _______, _______,                   _______,                            _______, _______, _______, OS_WORD_L, OS_DOC_DN, OS_WORD_R
  ),
};
// clang-format on

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;
    switch (keycode) {
        case OS_SHOT_CLIP: tap_code16(LCTL(LGUI(LSFT(KC_4)))); return false; // Ctrl+Cmd+Shift+4
        case OS_SHOT_FILE: tap_code16(LGUI(LSFT(KC_4)));       return false; // Cmd+Shift+4
        case OS_MCTL:      tap_code16(LCTL(KC_UP));            return false; // macOS Mission Control = Ctrl+Up
        case OS_SPOT:      tap_code16(LGUI(KC_SPC));           return false;
        case OS_WORD_L:    tap_code16(LALT(KC_LEFT));          return false;
        case OS_WORD_R:    tap_code16(LALT(KC_RGHT));          return false;
        case OS_DOC_UP:    tap_code16(LGUI(KC_UP));            return false;
        case OS_DOC_DN:    tap_code16(LGUI(KC_DOWN));          return false;
        case KNOB_MODE:
            knob_mode = (knob_mode == KNOB_SCROLL) ? KNOB_UNDO : KNOB_SCROLL;
            return false;
        // SIDE_TOGG -> Task 6
    }
    return true;
}

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index != 0) return false;
    if (get_highest_layer(layer_state) == _FN) {
        if (knob_mode == KNOB_SCROLL) {
            tap_code16(clockwise ? MS_WHLD : MS_WHLU); // MS_WH* are the current names; sent via tap_code16
        } else { // KNOB_UNDO (macOS combos for now; OS-aware in Task 4)
            tap_code16(clockwise ? LGUI(LSFT(KC_Z)) : LGUI(KC_Z)); // redo / undo
        }
    } else {
        tap_code(clockwise ? KC_VOLU : KC_VOLD);
    }
    return false;
}
