#pragma once

// Layer-tap (Caps = tap Caps / hold _FN): make the hold register fast when chording.
#define TAPPING_TERM 200
#define HOLD_ON_OTHER_KEY_PRESS

// --- RGB matrix ---
#define RGB_MATRIX_TIMEOUT 480000            // idle 8 min -> off (wakes on keypress)
#define RGB_MATRIX_SLEEP                     // off when USB suspends (Mac asleep)
#define RGB_MATRIX_KEYPRESSES                // required for reactive effects
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define RGB_MATRIX_DEFAULT_HUE 12            // salmon-ish
#define RGB_MATRIX_DEFAULT_SAT 165
#define RGB_MATRIX_DEFAULT_VAL 163           // ~64% brightness
