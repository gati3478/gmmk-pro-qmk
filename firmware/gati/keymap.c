// Copyright 2026 Gati Petriashvili (@gati3478)
// SPDX-License-Identifier: GPL-2.0-or-later

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
    KNOB_CCW, KNOB_CW,         // Fn-layer knob turn (encoder map)
};

// Fixed combos the user binds once in macOS (Dictation / Do-Not-Disturb).
#define MAC_DICT LCTL(LALT(LGUI(KC_D)))
#define MAC_DND  LCTL(LALT(LGUI(KC_N)))

enum knob_modes { KNOB_SCROLL, KNOB_UNDO };
static uint8_t knob_mode = KNOB_SCROLL;
static bool side_leds_on = true;

// Fn+B (QK_BOOT) fires only after a deliberate hold, so a stray chord can't drop the board
// into DFU mid-typing. Armed in process_record_user, fired from housekeeping_task_user.
#define BOOT_HOLD_MS 1000
static bool     boot_held = false;
static uint32_t boot_pressed_at;

// Keys whose Fn binding is a chord outside the typing block (media row, Shot, knob, End, arrows).
// Pressed during a pending Caps they settle it as hold at once, so a sloppy chord (Caps released
// first) is still Fn. Typing keys — the RGB number row, B (boot) and N (NKRO) included — don't,
// so switching language and typing on stays a Caps tap.
static bool is_fn_chord_key(uint16_t keycode) {
    switch (keycode) {
        case KC_F1 ... KC_F12:
        case OS_SHOT_CLIP:
        case KC_MPLY:              // knob press
        case KC_VOLD: case KC_VOLU: // knob turns (base-layer encoder map)
        case KC_END:
        case KC_LEFT: case KC_RGHT: case KC_UP: case KC_DOWN:
            return true;
        default:
            return false;
    }
}

// get_hold_on_other_key_press only sees the Caps key, so remember the key that interrupted it.
static uint16_t last_pressed_keycode = KC_NO;

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

// Detents are key events, so a turn during a pending Caps settles it as hold (is_fn_chord_key)
// and lands on _FN from the first click.
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
  [_BASE] = { ENCODER_CCW_CW(KC_VOLD,  KC_VOLU) },
  [_FN]   = { ENCODER_CCW_CW(KNOB_CCW, KNOB_CW) },
};
// clang-format on

static void knob_fn_turn(bool clockwise) {
    if (knob_mode == KNOB_SCROLL) {
        tap_code16(clockwise ? MS_WHLU : MS_WHLD); // clockwise = wheel up (flipped per feel)
    } else { // KNOB_UNDO
        uint16_t undo = is_mac() ? LGUI(KC_Z)        : LCTL(KC_Z);
        uint16_t redo = is_mac() ? LGUI(LSFT(KC_Z))  : LCTL(LSFT(KC_Z));
        tap_code16(clockwise ? redo : undo);
    }
}

// Runs on every raw event before the tap/hold decision, with the keycode of the current layer.
bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        last_pressed_keycode = keycode;
    }
    return true;
}

bool get_hold_on_other_key_press(uint16_t keycode, keyrecord_t *record) {
    return keycode == LT(_FN, KC_CAPS) && is_fn_chord_key(last_pressed_keycode);
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode == QK_BOOT) {
        boot_held       = record->event.pressed;
        boot_pressed_at = timer_read32();
        return false; // skip QMK's instant reset
    }
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
        case SIDE_TOGG:
            side_leds_on = !side_leds_on;
            return false;
        case KNOB_CCW:
        case KNOB_CW:
            knob_fn_turn(keycode == KNOB_CW);
            return false;
    }
    return true;
}

void housekeeping_task_user(void) {
    if (boot_held && timer_elapsed32(boot_pressed_at) >= BOOT_HOLD_MS) {
        reset_keyboard();
    }
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

// Side diffuser strips = a functional readout, repainted every frame (never latched).
// Priority: toggled off (Fn+9) > Caps Lock (red) > FN (sage scroll / amber undo) > base (off).
// Strips stay dark at rest and while typing (per-key salmon reactive is a separate LED set,
// untouched here). When the matrix is suspended/idle/off the framework skips indicators
// entirely, no guard needed.
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // Dark is painted, not skipped: otherwise an animated RGB effect shows through the strips.
    uint8_t r = 0, g = 0, b = 0;
    if (side_leds_on) {
        if (host_keyboard_led_state().caps_lock) {          // pulsing red (triangle wave, no lib deps)
            uint32_t t = timer_read32() % 1000u;  // 32-bit avoids the ~65 s wrap hiccup in the pulse
            r = (t < 500u) ? (uint8_t)(t * 255u / 500u) : (uint8_t)((1000u - t) * 255u / 500u);
        } else if (get_highest_layer(layer_state) == _FN) { // knob-mode color
            if (knob_mode == KNOB_SCROLL) { r = 100; g = 190; b = 110; } // sage (scroll)
            else                          { r = 255; g = 170; b = 40;  } // amber (undo)
        }
    }

    // rgb_matrix_set_color bypasses the global brightness; scale so Fn+5/6 dim the strips too.
    uint8_t v = rgb_matrix_get_val();
    r         = (uint16_t)r * v / 255;
    g         = (uint16_t)g * v / 255;
    b         = (uint16_t)b * v / 255;

    for (uint8_t i = led_min; i < led_max; i++) {
        if (HAS_FLAGS(g_led_config.flags[i], LED_FLAG_UNDERGLOW)) {
            rgb_matrix_set_color(i, r, g, b);
        }
    }
    return false;
}
