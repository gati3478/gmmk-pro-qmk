// Copyright 2026 Gati Petriashvili (@gati3478)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Layer-tap (Caps = tap Caps / hold _FN). Nav/media/knob keys settle it as hold on press
// (get_hold_on_other_key_press in keymap.c); for typing keys PERMISSIVE_HOLD applies: tapped
// inside the hold = Fn, a roll (Caps released first — switch language, start typing) = Caps tap.
#define TAPPING_TERM 200
#define PERMISSIVE_HOLD
#define HOLD_ON_OTHER_KEY_PRESS_PER_KEY
#define QUICK_TAP_TERM 0 // tap-then-hold = Fn, never a held Caps

// --- RGB matrix ---
#define RGB_MATRIX_TIMEOUT 480000            // idle 8 min -> off (wakes on keypress)
#define RGB_MATRIX_SLEEP                     // off when USB suspends (Mac asleep)
#define RGB_MATRIX_KEYPRESSES                // required for reactive effects
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define RGB_MATRIX_DEFAULT_HUE 12            // salmon-ish
#define RGB_MATRIX_DEFAULT_SAT 165
#define RGB_MATRIX_DEFAULT_VAL 163           // ~64% brightness
