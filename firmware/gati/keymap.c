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

static bool is_mac(void) {
    return detected_host_os() != OS_LINUX; // default to macOS when unsure
}

// DFU flashing never erases EEPROM, so a stale saved RGB effect would shadow the
// RGB_MATRIX_DEFAULT_* in config.h. Stamp a version in the 4-byte user EEPROM block;
// when it changes, re-apply the salmon-reactive defaults exactly once (bump to redo later).
#define GATI_EECONFIG_VERSION 1
typedef union {
    uint32_t raw;
    struct { uint8_t version; };
} gati_config_t;
static gati_config_t gati_config;

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
        case OS_SHOT_CLIP:
            tap_code16(is_mac() ? LCTL(LGUI(LSFT(KC_4))) : LCTL(LSFT(KC_PSCR))); return false;
        case OS_SHOT_FILE:
            tap_code16(is_mac() ? LGUI(LSFT(KC_4)) : LSFT(KC_PSCR)); return false;
        case OS_MCTL:
            if (is_mac()) tap_code16(LCTL(KC_UP)); else tap_code(KC_LGUI); return false; // Mac Ctrl+Up / Linux Super=overview
        case OS_SPOT:
            if (is_mac()) tap_code16(LGUI(KC_SPC)); else tap_code(KC_LGUI); return false; // Linux: Super=search
        case OS_WORD_L:
            tap_code16(is_mac() ? LALT(KC_LEFT) : LCTL(KC_LEFT)); return false;
        case OS_WORD_R:
            tap_code16(is_mac() ? LALT(KC_RGHT) : LCTL(KC_RGHT)); return false;
        case OS_DOC_UP:
            tap_code16(is_mac() ? LGUI(KC_UP)   : LCTL(KC_HOME)); return false;
        case OS_DOC_DN:
            tap_code16(is_mac() ? LGUI(KC_DOWN) : LCTL(KC_END));  return false;
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
            tap_code16(clockwise ? MS_WHLU : MS_WHLD); // clockwise = wheel up (flipped per feel); MS_WH* are current names
        } else { // KNOB_UNDO
            uint16_t undo = is_mac() ? LGUI(KC_Z)        : LCTL(KC_Z);
            uint16_t redo = is_mac() ? LGUI(LSFT(KC_Z))  : LCTL(LSFT(KC_Z));
            tap_code16(clockwise ? redo : undo);
        }
    } else {
        tap_code(clockwise ? KC_VOLU : KC_VOLD);
    }
    return false;
}

void keyboard_post_init_user(void) {
    gati_config.raw = eeconfig_read_user();
    if (gati_config.version != GATI_EECONFIG_VERSION) {
        rgb_matrix_enable();                                   // ensure RGB on
        rgb_matrix_mode(RGB_MATRIX_SOLID_REACTIVE_SIMPLE);     // salmon bloom (persisted)
        rgb_matrix_sethsv(RGB_MATRIX_DEFAULT_HUE, RGB_MATRIX_DEFAULT_SAT, RGB_MATRIX_DEFAULT_VAL);
        gati_config.version = GATI_EECONFIG_VERSION;
        eeconfig_update_user(gati_config.raw);
    }
}
