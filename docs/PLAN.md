# GMMK Pro QMK — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Because flashing is hardware-in-the-loop (Gati presses the flash combo and verifies on the physical board), this plan is meant for **inline execution in the live session**, not autonomous subagents.

**Goal:** Build, from scratch, a 2-layer mainline-QMK firmware for the GMMK Pro tuned for macOS (primary) and Fedora/GNOME (secondary), with an OS-aware action set, a modal rotary knob, reactive-salmon per-key RGB, and a functional side-LED state machine — all versioned in this repo as the single source of truth.

**Architecture:** The firmware lives in `firmware/gati/` here; `scripts/flash.sh` symlinks it into a local `~/qmk_firmware` clone and drives `qmk compile` / `qmk flash`. Behavior is built incrementally: each task adds one capability, compiles clean, and is flashed + verified on hardware before the next. The full key `LAYOUT` is written once (Task 2) and never edited again — later tasks only add handler functions (`process_record_user`, `encoder_update_user`, `rgb_matrix_indicators_advanced_user`) and config.

**Tech Stack:** QMK (mainline), C, ChibiOS/STM32F303, `qmk` CLI via Homebrew, ARM GCC toolchain, `dfu-util`.

## Global Constraints

- Keyboard target: `gmmk/pro/ansi`; keymap name: `gati`. Compile/flash always as `-kb gmmk/pro/ansi -km gati`.
- Exactly **two layers**: `_BASE` (0), `_FN` (1). Do not add more.
- **Keycodes must match keycap legends** — never map a key to something its printed cap doesn't say.
- Pure mainline QMK, **no VIA/Vial**.
- Platforms: macOS primary, Fedora/GNOME secondary, **no Windows**. When OS detection is unsure, default to macOS behavior.
- Bootloader entry for every flash: **Fn+B** (`QK_BOOT`) or **hold Esc while plugging in** (bootmagic).
- RGB: effect `SOLID_REACTIVE_SIMPLE`, salmon hot color `RGB(255,120,90)`, brightness value `163` (~64%), idle timeout `480000` ms (8 min), off on USB suspend.
- Side-strip priority: `suspend → off` ▸ `Caps Lock → pulsing red` ▸ `FN → knob-mode color (teal scroll / amber undo)` ▸ `base → salmon`.
- Commit after every task. Never commit `.build/`.

---

### Task 1: QMK toolchain + stock compile (environment proof)

Proves the Mac can build GMMK Pro firmware before we write any custom code.

**Files:** none in-repo (installs tooling, clones `~/qmk_firmware`).

- [ ] **Step 1: Install the QMK CLI**

Run:

```bash
brew install qmk/qmk/qmk
```

Expected: `qmk` on PATH. Verify: `qmk --version` prints a version.

- [ ] **Step 2: Run qmk setup (clones qmk_firmware + ARM toolchain)**

Run:

```bash
qmk setup -y
```

Expected: clones `~/qmk_firmware`, installs `arm-none-eabi-gcc` and `dfu-util`. Verify: `qmk doctor` ends with `QMK is ready to go`.

- [ ] **Step 3: Compile the stock GMMK Pro firmware**

Run:

```bash
qmk compile -kb gmmk/pro/ansi -km default
```

Expected: ends with `[OK]` and writes `gmmk_pro_ansi_default.bin`. This confirms the toolchain works end to end.

- [ ] **Step 4: Record the exact LAYOUT skeleton for reference**

Run:

```bash
sed -n '/LAYOUT(/,/)/p' ~/qmk_firmware/keyboards/gmmk/pro/ansi/keymaps/default/keymap.c | head -20
```

Expected: shows the 83-key `LAYOUT(...)` macro grouped 15/15/15/14/14/10. Confirm the argument count matches this plan's Task 2 layers. If the skeleton differs, note the difference before Task 2.

_(No commit — nothing added to the repo yet.)_

---

### Task 2: Firmware scaffold + full keymap (Mac behavior) + flash pipeline

First flashable firmware: both layers complete, all standard keys + OS-action custom keycodes behaving as macOS combos. The `LAYOUT` written here is final and untouched by later tasks.

**Files:**

- Create: `firmware/gati/keymap.c`
- Create: `firmware/gati/config.h`
- Create: `firmware/gati/rules.mk`
- Create: `firmware/gati/readme.md`
- Create: `scripts/flash.sh`
- Modify: `README.md` (add Build & Flash section)

**Interfaces:**

- Produces: `enum custom_keycodes { OS_SHOT_CLIP=SAFE_RANGE, OS_SHOT_FILE, OS_MCTL, OS_SPOT, OS_WORD_L, OS_WORD_R, OS_DOC_UP, OS_DOC_DN, KNOB_MODE, SIDE_TOGG };` — later tasks add handlers for `KNOB_MODE` (Task 3), the OS branch inside `OS_*` (Task 4), and `SIDE_TOGG` (Task 6).
- Produces: layer enum `enum layers { _BASE, _FN };`.

- [ ] **Step 1: Create `firmware/gati/rules.mk`**

```make
NKRO_ENABLE = yes        # Fn+N = NK_TOGG
# RGB_MATRIX is enabled by the gmmk/pro/ansi board defaults.
# MOUSEKEY (knob scroll) and OS_DETECTION are added in later tasks.
```

- [ ] **Step 2: Create `firmware/gati/config.h` (minimal for now)**

```c
#pragma once

// Layer-tap (Caps = tap Caps / hold _FN): make the hold register fast when chording.
#define TAPPING_TERM 200
#define HOLD_ON_OTHER_KEY_PRESS
```

- [ ] **Step 3: Create `firmware/gati/keymap.c` (both layers, macOS behavior)**

```c
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
    RGB_TOG, RGB_MOD, RGB_RMOD,RGB_HUI, RGB_SAI, RGB_VAI, RGB_VAD, RGB_SPI, RGB_SPD, SIDE_TOGG,_______, _______, _______, _______,      _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,               KC_HOME,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,      _______,
    _______, _______, _______, _______, _______, QK_BOOT, NK_TOGG, _______, _______, _______, _______, _______, OS_DOC_UP,             _______,
    _______, _______, _______,                   _______,                            _______, _______, _______, OS_WORD_L, OS_DOC_DN, OS_WORD_R
  ),
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;
    switch (keycode) {
        case OS_SHOT_CLIP: tap_code16(LCTL(LGUI(LSFT(KC_4)))); return false; // Ctrl+Cmd+Shift+4
        case OS_SHOT_FILE: tap_code16(LGUI(LSFT(KC_4)));       return false; // Cmd+Shift+4
        case OS_MCTL:      tap_code(KC_MCTL);                  return false;
        case OS_SPOT:      tap_code16(LGUI(KC_SPC));           return false;
        case OS_WORD_L:    tap_code16(LALT(KC_LEFT));          return false;
        case OS_WORD_R:    tap_code16(LALT(KC_RGHT));          return false;
        case OS_DOC_UP:    tap_code16(LGUI(KC_UP));            return false;
        case OS_DOC_DN:    tap_code16(LGUI(KC_DOWN));          return false;
        // KNOB_MODE -> Task 3, SIDE_TOGG -> Task 6
    }
    return true;
}
```

- [ ] **Step 4: Create `firmware/gati/readme.md`**

```markdown
# gati — GMMK Pro keymap

Mainline-QMK keymap for macOS (primary) / Fedora (secondary). See `../../docs/DESIGN.md`
for the full design. Build/flash via `../../scripts/flash.sh`.
```

- [ ] **Step 5: Create `scripts/flash.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QMK_KM="$HOME/qmk_firmware/keyboards/gmmk/pro/ansi/keymaps/gati"

mkdir -p "$(dirname "$QMK_KM")"
ln -sfn "$REPO/firmware/gati" "$QMK_KM"   # repo stays the source of truth

case "${1:-flash}" in
  compile) qmk compile -kb gmmk/pro/ansi -km gati ;;
  flash)   qmk flash   -kb gmmk/pro/ansi -km gati ;;
  *) echo "usage: flash.sh [compile|flash]"; exit 1 ;;
esac
```

Then: `chmod +x scripts/flash.sh`

- [ ] **Step 6: Compile (the test)**

Run:

```bash
./scripts/flash.sh compile
```

Expected: `[OK]` and `gmmk_pro_ansi_gati.bin` produced. If it errors on `LAYOUT` argument count, diff against `~/qmk_firmware/keyboards/gmmk/pro/ansi/keymaps/default/keymap.c` and align the row grouping (15/15/15/14/14/10).

- [ ] **Step 7: Add the Build & Flash section to `README.md`**

Append:

````markdown
## Build & flash

```bash
./scripts/flash.sh compile     # build only
./scripts/flash.sh flash       # build + flash (put board in bootloader first)
```
````

**Enter bootloader:** press **Fn+B**, or hold **Esc** while plugging in the cable.
Requires `brew install qmk/qmk/qmk` + `qmk setup` once (see docs/DESIGN.md §12).

````

- [ ] **Step 8: First real flash + hardware verification**

Put the board in bootloader (hold Esc, replug — this is the first flash so Fn+B isn't on the board yet), then:
```bash
./scripts/flash.sh flash
````

Hardware checklist (verify on Mac):

- All alphas/numbers/symbols type correctly; Enter/Backspace/Tab/Shift work.
- Bottom row: key left of space = ⌘ (test ⌘C/⌘V); Ctrl·Opt·Cmd left, Ctrl·Opt·Fn right.
- Caps: quick tap switches input source (needs the macOS setting from Task 7); hold + F1/F2 changes brightness.
- Fn (hold Caps or the Fn key) + F7/F8/F9 = media prev/play/next; F11/F12 = volume.
- Fn+End = Home; Fn+arrows = word/doc nav; top-right key = screenshot-to-clipboard (paste into Notes).
- Knob turn = volume, press = play/pause. (Fn+knob does nothing yet — that's Task 3.)

- [ ] **Step 9: Commit**

```bash
git add firmware/gati scripts/flash.sh README.md
git commit -m "firmware: full 2-layer keymap (macOS behavior) + flash pipeline"
```

---

### Task 3: Modal knob (scroll ↔ undo/redo)

Adds the Fn-layer knob behavior and the mode toggle.

**Files:**

- Modify: `firmware/gati/rules.mk` (add MOUSEKEY)
- Modify: `firmware/gati/keymap.c` (add knob state, `KNOB_MODE` handler, `encoder_update_user`)

**Interfaces:**

- Produces: `enum knob_modes { KNOB_SCROLL, KNOB_UNDO };` and file-scope `static uint8_t knob_mode = KNOB_SCROLL;` — read by the side-LED indicators in Task 6.

- [ ] **Step 1: Enable mousekeys in `rules.mk`**

Change `rules.mk` to:

```make
NKRO_ENABLE     = yes    # Fn+N = NK_TOGG
MOUSEKEY_ENABLE = yes    # knob scroll (KC_MS_WH_UP/DOWN)
```

- [ ] **Step 2: Add knob state + mode toggle in `keymap.c`**

Directly under the `#define MAC_DND ...` line, add:

```c
enum knob_modes { KNOB_SCROLL, KNOB_UNDO };
static uint8_t knob_mode = KNOB_SCROLL;
```

In `process_record_user`, add this case (before the closing `}` of the switch):

```c
        case KNOB_MODE:
            knob_mode = (knob_mode == KNOB_SCROLL) ? KNOB_UNDO : KNOB_SCROLL;
            return false;
```

- [ ] **Step 3: Add `encoder_update_user` at the end of `keymap.c`**

```c
bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index != 0) return false;
    if (get_highest_layer(layer_state) == _FN) {
        if (knob_mode == KNOB_SCROLL) {
            tap_code(clockwise ? KC_MS_WH_DOWN : KC_MS_WH_UP);
        } else { // KNOB_UNDO (macOS combos for now; OS-aware in Task 4)
            tap_code16(clockwise ? LGUI(LSFT(KC_Z)) : LGUI(KC_Z)); // redo / undo
        }
    } else {
        tap_code(clockwise ? KC_VOLU : KC_VOLD);
    }
    return false;
}
```

- [ ] **Step 4: Compile (the test)**

Run: `./scripts/flash.sh compile`
Expected: `[OK]`.

- [ ] **Step 5: Flash + verify**

Enter bootloader (now **Fn+B** works), `./scripts/flash.sh flash`. Verify:

- Base knob: turn = volume, press = play/pause (unchanged).
- Hold Fn + turn knob = page scrolls (test in a browser / terminal scrollback).
- Hold Fn + press knob (mode → undo), then hold Fn + turn = undo/redo (test in a text editor).
- Hold Fn + press knob again returns to scroll.
- If scroll direction feels inverted, flip the `clockwise ?` ternary in Step 3 (one-line change).

- [ ] **Step 6: Commit**

```bash
git add firmware/gati
git commit -m "firmware: modal knob (scroll <-> undo/redo, Fn+press cycles)"
```

---

### Task 4: OS-aware actions (macOS ⇄ Fedora)

Turns on host-OS detection and branches every action that differs between macOS and Fedora/GNOME. `LAYOUT` is untouched — only handler bodies change.

**Files:**

- Modify: `firmware/gati/rules.mk` (add OS_DETECTION)
- Modify: `firmware/gati/keymap.c` (add `is_mac()`, branch `OS_*` + knob undo/redo)

**Interfaces:**

- Produces: `static bool is_mac(void);` — true for macOS/unsure/windows/iOS, false only for Linux.

- [ ] **Step 1: Enable OS detection in `rules.mk`**

```make
NKRO_ENABLE         = yes
MOUSEKEY_ENABLE     = yes
OS_DETECTION_ENABLE = yes
```

- [ ] **Step 2: Add the `is_mac()` helper in `keymap.c`**

Under the `knob_mode` declaration, add:

```c
static bool is_mac(void) {
    return detected_host_os() != OS_LINUX; // default to macOS when unsure
}
```

- [ ] **Step 3: Replace the `OS_*` cases in `process_record_user`**

Replace the eight `OS_*` cases with OS-branched versions:

```c
        case OS_SHOT_CLIP:
            tap_code16(is_mac() ? LCTL(LGUI(LSFT(KC_4))) : LCTL(LSFT(KC_PSCR))); return false;
        case OS_SHOT_FILE:
            tap_code16(is_mac() ? LGUI(LSFT(KC_4)) : LSFT(KC_PSCR)); return false;
        case OS_MCTL:
            if (is_mac()) tap_code(KC_MCTL); else tap_code(KC_LGUI); return false; // Linux: Super=overview
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
```

- [ ] **Step 4: OS-branch the knob undo/redo in `encoder_update_user`**

Replace the `KNOB_UNDO` line with:

```c
        } else { // KNOB_UNDO
            uint16_t undo = is_mac() ? LGUI(KC_Z)        : LCTL(KC_Z);
            uint16_t redo = is_mac() ? LGUI(LSFT(KC_Z))  : LCTL(LSFT(KC_Z));
            tap_code16(clockwise ? redo : undo);
        }
```

- [ ] **Step 5: Compile (the test)**

Run: `./scripts/flash.sh compile`
Expected: `[OK]`.

- [ ] **Step 6: Flash + verify (Mac now; Fedora later)**

Flash, then confirm on Mac nothing regressed: screenshot-to-clipboard, Fn+arrows word/doc nav, Fn+F3 Mission Control, Fn+F4 Spotlight, knob undo/redo all behave as before. (Fedora verification is deferred until the Linux box is set up; the branches are already in place.)
If `os_detection` misreads behind a hub/KVM, add `#define OS_DETECTION_DEBOUNCE 250` to `config.h` and reflash.

- [ ] **Step 7: Commit**

```bash
git add firmware/gati
git commit -m "firmware: OS-aware actions (macOS/Fedora) via os_detection"
```

---

### Task 5: Reactive-salmon per-key RGB + sleep/idle

Sets the default lighting: dark base, salmon bloom on keypress, off when asleep or idle.

**Files:**

- Modify: `firmware/gati/config.h`

- [ ] **Step 1: Append RGB config to `config.h`**

```c
// --- RGB matrix ---
#define RGB_MATRIX_TIMEOUT 480000            // idle 8 min -> off (wakes on keypress)
#define RGB_MATRIX_SLEEP                     // off when USB suspends (Mac asleep)
#define RGB_MATRIX_KEYPRESSES                // required for reactive effects
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define RGB_MATRIX_DEFAULT_HUE 12            // salmon-ish
#define RGB_MATRIX_DEFAULT_SAT 165
#define RGB_MATRIX_DEFAULT_VAL 163           // ~64% brightness
```

- [ ] **Step 2: Compile (the test)**

Run: `./scripts/flash.sh compile`
Expected: `[OK]`. If `RGB_MATRIX_SLEEP` is unknown on this QMK version, replace with `#define RGB_DISABLE_WHEN_USB_SUSPENDED` and recompile.

- [ ] **Step 3: Flash + verify**

Flash, then verify:

- Board is dark at rest; pressing keys blooms **salmon** underglow that fades.
- Brightness looks ~medium (tune with Fn+5/Fn+6 live; the default is 64%).
- Let it sit 8 min → LEDs fade off; a keypress wakes them.
- Sleep the Mac → LEDs go off; wake → they return.

- [ ] **Step 4: Commit**

```bash
git add firmware/gati
git commit -m "firmware: reactive-salmon RGB + idle/sleep off"
```

---

### Task 6: Side-LED state machine

Overrides the side diffuser LEDs each frame with the functional indicator scheme.

**Files:**

- Modify: `firmware/gati/keymap.c` (add `side_leds_on`, `SIDE_TOGG` handler, `rgb_matrix_indicators_advanced_user`)

**Interfaces:**

- Consumes: `knob_mode` (Task 3), `_FN` layer, `is_mac()` not needed here.

- [ ] **Step 1: Add the side-LED toggle state + handler**

Under the `knob_mode` declaration in `keymap.c`, add:

```c
static bool side_leds_on = true;
```

In `process_record_user`, add a case:

```c
        case SIDE_TOGG:
            side_leds_on = !side_leds_on;
            return false;
```

- [ ] **Step 2: Add the indicators function at the end of `keymap.c`**

```c
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (!side_leds_on || rgb_matrix_get_suspend_state()) return false;

    uint8_t r, g, b;
    if (host_keyboard_led_state().caps_lock) {          // pulsing red
        uint8_t p = sin8((uint8_t)(timer_read() >> 2));
        r = p; g = 0; b = 0;
    } else if (get_highest_layer(layer_state) == _FN) { // knob-mode color
        if (knob_mode == KNOB_SCROLL) { r = 60;  g = 200; b = 200; } // teal
        else                          { r = 255; g = 170; b = 40;  } // amber
    } else {                                            // base
        r = 255; g = 120; b = 90;                        // salmon
    }

    for (uint8_t i = led_min; i < led_max; i++) {
        if (HAS_FLAGS(g_led_config.flags[i], LED_FLAG_UNDERGLOW)) {
            rgb_matrix_set_color(i, r, g, b);
        }
    }
    return false;
}
```

- [ ] **Step 3: Compile (the test)**

Run: `./scripts/flash.sh compile`
Expected: `[OK]`. If `sin8` is undefined, add `#include "lib/lib8tion/lib8tion.h"` near the top of `keymap.c`. If no side LEDs light in Step 4, the board may not flag them `LED_FLAG_UNDERGLOW` — read `~/qmk_firmware/keyboards/gmmk/pro/ansi/ansi.c` `g_led_config`, identify the left/right edge LED indices, and replace the `HAS_FLAGS(...)` check with an explicit index list.

- [ ] **Step 4: Flash + verify**

Flash, then verify the side strips:

- At rest: **salmon**.
- Hold Fn (or Caps-hold): **teal**; press Fn+knob to switch mode → **amber**; live-changes as you cycle.
- Long-press Caps to toggle real Caps Lock ON: strips **pulse red**; toggle off → back to salmon.
- Fn+9 (`SIDE_TOGG`) turns the strips off/on.

- [ ] **Step 5: Commit**

```bash
git add firmware/gati
git commit -m "firmware: functional side-LED state machine (caps/layer/knob-mode)"
```

---

### Task 7: Docs — CLAUDE.md, README, macOS settings checklist

Finalizes the repo as a self-contained SSOT anyone (including future-you) can rebuild from.

**Files:**

- Create: `CLAUDE.md`
- Create: `docs/macos-setup.md`
- Modify: `README.md` (link the settings checklist)

- [ ] **Step 1: Create `CLAUDE.md`**

```markdown
# CLAUDE.md — GMMK Pro QMK

Personal QMK firmware for a Glorious GMMK Pro (75% ANSI), macOS-primary / Fedora-secondary.

## Source of truth

- Design: `docs/DESIGN.md` (authoritative — update it before changing behavior).
- Firmware: `firmware/gati/{keymap.c,config.h,rules.mk}` — pure mainline QMK, 2 layers (`_BASE`,`_FN`).
- Build/flash: `scripts/flash.sh` (symlinks `firmware/gati` into `~/qmk_firmware`).

## Rules

- Exactly two layers. Keep it that way.
- **Keycodes must match keycap legends** — never map a key to something its cap doesn't say.
- macOS primary, Fedora secondary, no Windows. OS-differing actions branch on `detected_host_os()`.
- Enter bootloader to flash: **Fn+B** or hold **Esc** while plugging in.
- After any behavior change: update `docs/DESIGN.md`, `./scripts/flash.sh compile`, flash, verify, commit.

## Layout at a glance

See `docs/DESIGN.md` §5–§8 for the full base/Fn maps, modal knob, and RGB/side-LED scheme.
```

- [ ] **Step 2: Create `docs/macos-setup.md`**

```markdown
# macOS one-time setup

1. **Caps Lock switches input source** — System Settings ▸ Keyboard ▸ Keyboard Shortcuts ▸
   (or the "Caps Lock key" row under Input Sources): set Caps Lock to switch to/from your
   non-Latin source (Georgian). Now a Caps tap on the GMMK cycles input like on the laptop.
2. **Dictation (Fn+F5)** — System Settings ▸ Keyboard ▸ Dictation ▸ Shortcut ▸ Customize,
   then press **⌃⌥⌘D** (Ctrl+Option+Cmd+D) so it matches `MAC_DICT`.
3. **Do Not Disturb (Fn+F6)** — create a Shortcuts.app shortcut that toggles a Focus, then
   assign it the hotkey **⌃⌥⌘N** (Ctrl+Option+Cmd+N) to match `MAC_DND`.
4. **Function keys** — external keyboards send real F-keys by default (what we want); no change needed.
```

- [ ] **Step 3: Link the checklist from `README.md`**

Append to README:

```markdown
## macOS settings

One-time setup (Caps→input-switch, Dictation, DND): see [`docs/macos-setup.md`](docs/macos-setup.md).
```

- [ ] **Step 4: Commit**

```bash
git add CLAUDE.md docs/macos-setup.md README.md
git commit -m "docs: CLAUDE.md, macOS setup checklist, README links"
```

---

## Self-Review

**Spec coverage:**

- §5 base layer → Task 2 `_BASE`. ✓
- §6 Fn layer (F-row, RGB row, nav, boot, NKRO) → Task 2 `_FN`. ✓
- §7 modal knob → Task 3. ✓
- OS-aware actions (§5 Shot, §6 nav, §9 keycodes) → Task 2 (Mac) + Task 4 (branch). ✓
- §8.1 per-key RGB → Task 5. ✓
- §8.2 side-LED state machine → Task 6. ✓
- §10 rules/config → distributed: NKRO (T2), MOUSEKEY (T3), OS_DETECTION (T4), RGB (T5). ✓
- §12 toolchain → Task 1. ✓
- §13 macOS settings → Task 7. ✓
- Bootloader Fn+B → Task 2 `_FN[64]=QK_BOOT`. ✓

**Placeholder scan:** No TBD/TODO; every code step shows complete content. ✓

**Type consistency:** `knob_mode` (uint8_t, `KNOB_SCROLL/KNOB_UNDO`), `side_leds_on` (bool), `is_mac()` (bool), custom-keycode enum names — all referenced consistently across Tasks 2/3/4/6. `LAYOUT` written once in Task 2, never re-edited. ✓

**Known implementation risks (flagged with fixes inline):** `LAYOUT` arg-count drift (Task 2 Step 6), `RGB_MATRIX_SLEEP` naming (Task 5 Step 2), `LED_FLAG_UNDERGLOW` targeting the wrong LEDs (Task 6 Step 3), `os_detection` flakiness (Task 4 Step 6), scroll direction (Task 3 Step 5).
