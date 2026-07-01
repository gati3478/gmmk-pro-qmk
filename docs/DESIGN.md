# GMMK Pro — QMK Design Spec

**Date:** 2026-07-01
**Owner:** Gati
**Status:** Approved — implementation plan written (`docs/PLAN.md`); firmware build not yet started
**Firmware base:** Pure mainline QMK (hand-written C). This repository is the single source of truth.

---

## 1. Goal

A polished, purposeful QMK setup for a GMMK Pro used daily on **macOS (primary)** and **Fedora/GNOME Linux (secondary)** for writing, browsing, chatting, terminal work, programming, and Claude Code / "vibe coding". Not barebones, not a tower of meta-layers — **exactly two layers**, every binding earning its place.

Non-goals: Windows support, VIA/Vial GUI remapping (chosen: pure C source in git), RGB spectacle for its own sake.

---

## 2. Hardware (as-built)

| Part             | Detail                                                                   |
| ---------------- | ------------------------------------------------------------------------ |
| Board            | Glorious GMMK Pro, 75% ANSI, white                                       |
| MCU / bootloader | STM32F303 (Cortex-M4, ChibiOS) · **stm32-dfu** bootloader                |
| Switches         | Kailh BOX Silent Brown (tactile, silent; 45g act / 65g peak)             |
| Plate            | **Polycarbonate** (swapped from stock aluminium) — softer, deeper sound  |
| Stabilizers      | Premium (upgraded from stock), lubed                                     |
| Dampening        | O-rings installed                                                        |
| Keycaps          | PBT 9009-style, Cherry profile, **non-shine-through**, cream/sage/salmon |
| Knob             | Glorious gold rotary (clickable encoder)                                 |
| Cable            | Coiled USB-C (KBDfans)                                                   |
| RGB              | Per-key south-facing + left/right side diffuser strips                   |

**Consequence that drives the RGB design:** PBT + south-facing LEDs ⇒ legends never light; per-key color reads as **edge/underglow**, and the **side strips are the dominant visual + information surface**.

---

## 3. Recovered from the old firmware (forensics)

Reverse-engineered from the un-stripped `.elf` + `.map` + the `layout_mine.json` left in this folder:

- Old firmware was **plain mainline QMK**, built on Windows (`C:\Users\gatip\qmk_firmware`), keymap `keyboards/gmmk/pro/ansi/keymaps/gati3478`. No VIA/Vial — so any change required a recompile on a now-gone PC. (This repo + git fixes that permanently.)
- Old **bootloader combo** was `Fn + \` and `Fn + Del` (`RESET`). Media/RGB lived on the `MO(1)` Fn layer; NKRO on `Fn+N`. Caps Lock drove the side glow (source of the "stuck lit" bug).
- RGB was `RGB_MATRIX` with keypress reactive effects (heatmap confirmed) + cycle/gradient/breathing.

These artifacts are preserved under `reference/` for provenance; **none of the old code is reused** — the new firmware is written from scratch.

---

## 4. Architecture overview

- **Layers (2):** `_BASE` (0), `_FN` (1).
- **`_FN` entry (two ways):** hold **Caps** (`LT(_FN, KC_CAPS)`) _or_ the dedicated **Fn** key (`MO(_FN)`).
- **OS awareness:** `OS_DETECTION_ENABLE`. A small helper resolves dual-behavior actions to the right combo for macOS vs Linux at runtime; unknown host defaults to macOS (primary machine wins ties).
- **Custom code lives in** `keymap.c`: OS-aware action keycodes, the modal knob, and the side-LED state machine in `rgb_matrix_indicators_advanced_user`.

---

## 5. Base layer (`_BASE`)

```
┌────┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬────┐  ╭────╮
│Esc │F1 │F2 │F3 │F4 │F5 │F6 │F7 │F8 │F9 │F10│F11│F12│Shot│  │KNOB│ turn=Vol
├────┼───┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬───┤  ╰────╯ push=Play/Pause
│ `  │ 1  │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │ 0 │ - │ = │Bsp │  │Del │
├────┴┬───┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬───┤  ├────┤
│ Tab │ Q  │ W │ E │ R │ T │ Y │ U │ I │ O │ P │ [ │ ] │ \  │  │End │
├─────┴┬───┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴───┴───┤  ├────┤
│ CAPS │ A  │ S │ D │ F │ G │ H │ J │ K │ L │ ; │ '  Enter │  │PgUp│
│⇪tap / FN hold                                            │  │    │
├──────┴┬───┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴──┬──┬────┤  ├────┤
│ Shift │ Z  │ X │ C │ V │ B │ N │ M │ , │ . │  /  │Sf│ ↑  │  │PgDn│
├────┬──┴┬───┴┬──┴───┴───┴───┴───┴──┬┴──┬┴──┬┴──┬──┼──┴┬───┼──┴────┤
│Ctrl│Opt│Cmd │       Space        │Ctl│Opt│Fn │  │ ← │ ↓ │  →    │
└────┴───┴────┴────────────────────┴───┴───┴───┴──┴───┴───┴───────┘
    Ctrl · Opt · Cmd │ space │ Ctrl · Opt · Fn   (⌘ left of space mirrors the MacBook; right stays honest Ctrl to match its cap)
```

Deviations from stock (everything else is standard ANSI):

| Key                             | Behavior                                   | Notes                                                                                                                                                                            |
| ------------------------------- | ------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Caps**                        | `LT(_FN, KC_CAPS)`                         | Tap = Caps Lock (macOS **Caps→switch input source** setting handles GE⇄EN); hold = `_FN`. A hold never emits Caps, so no accidental caps toggle.                                 |
| **Shot** (top-right, was PrtSc) | OS-aware **screenshot region → clipboard** | Mac `⌃⌘⇧4`, Linux/GNOME `Ctrl+Shift+PrtSc`. Optimized for capture→paste into Claude/chat.                                                                                        |
| **Knob**                        | turn = Volume ± · push = Play/Pause        |                                                                                                                                                                                  |
| Bottom-left mods                | `LCTL · LALT(Opt) · LGUI(Cmd)`             | Cmd immediately left of space (matches MacBook). Caps: `Ctrl · Alt · Code`.                                                                                                      |
| Bottom-right mods               | `RCTL · RALT(Opt) · MO(_FN)`               | Right of space = Ctrl · Opt · Fn; Fn rightmost (next to arrows) for one-handed Fn+arrow / Fn+knob. Keycaps match keycodes exactly (no mislabeled keys). Caps: `Ctrl · Alt · Fn`. |
| Right column                    | `Del · End · PgUp · PgDn`                  | Standard nav stack; already correctly capped.                                                                                                                                    |

---

## 6. Fn layer (`_FN`)

Hold Caps or Fn. Unlisted keys are transparent.

```
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐ ╭────╮
│ ·  │Bri-│Bri+│MsnC│Spot│Dict│ DND│Prev│Play│Next│Mute│Vol-│Vol+│ShotF│ │KNB│ turn: knob-mode
│    │ F1 │ F2 │ F3 │ F4 │ F5 │ F6 │ F7 │ F8 │ F9 │F10 │F11 │F12 │file │ ╰───╯ push: cycle mode
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼─────┤ │ · │
│RGB │Eff+│Eff-│Hue │Sat │Bri+│Bri-│Spd+│Spd-│Side│ ·  │ ·  │ ·  │     │ ├───┤
│tog │  1 │  2 │  3 │  4 │  5 │  6 │  7 │  8 │LED9│    │    │    │     │ │Hom│ ← Fn+End
├────┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─────┴──┐  ├───┤
│  ·   │ ·  │ ·  │ ·  │ ·  │ ·  │ ·  │ ·  │ ·  │ ·  │ ·  │    ·       │  │ · │ Fn+PgUp (stays PgUp)
├──────┴──┬─┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴─┬──┴──┬───────┤  ├───┤
│         │ · │ ·  │ ·  │ ·  │BOOT│NKro│ ·  │ ·  │ ·  │  ·  │        │  │ · │ Fn+PgDn (stays PgDn)
├─────────┼───┼────┼────┼────┼────┼────┼────┼────┼────┼──┬──┴─┬──┬──┤  ┌───┐
│         │ · │ ·  │ ·  │ ·  │ ·  │ ·  │ ·  │ ·  │ ·  │  │doc↑│  │  │  │d↑ │ Fn+↑ = doc top
├────┬────┼───┴┬───┴────┴────┴────┴────┴──┬─┴──┬─┴──┬─┴┬─┴──┬─┴──┼──┤  └───┘
│    │    │    │                          │    │    │  │word│d↓  │wrd│ Fn+←/→ = word, Fn+↓ = doc bot
└────┴────┴────┴──────────────────────────┴────┴────┴──┴────┴────┴──┘
```

### 6.1 Fn F-row — mirrors the actual M2 MacBook Pro F-row

| Key                | Action                       | Keycode / method                                                                |
| ------------------ | ---------------------------- | ------------------------------------------------------------------------------- |
| Fn+F1 / F2         | Brightness − / +             | `KC_BRID` / `KC_BRIU`                                                           |
| Fn+F3              | Mission Control              | OS-aware: Mac `⌃↑` (default Mission Control shortcut), Linux tap `Super`        |
| Fn+F4              | Spotlight / search           | OS-aware: Mac `⌘Space`, Linux `Super`                                           |
| Fn+F5              | **Dictation**                | Emits a fixed combo (`Ctrl+Opt+Cmd+D`); user binds macOS Dictation → same combo |
| Fn+F6              | **Do Not Disturb / Focus**   | Emits a fixed combo (`Ctrl+Opt+Cmd+N`); user binds via macOS Shortcuts.app      |
| Fn+F7 / F8 / F9    | Prev / Play / Next           | `KC_MPRV` / `KC_MPLY` / `KC_MNXT`                                               |
| Fn+F10 / F11 / F12 | Mute / Vol− / Vol+           | `KC_MUTE` / `KC_VOLD` / `KC_VOLU`                                               |
| Fn+Shot            | Screenshot region → **file** | Mac `⌘⇧4`, Linux `Shift+PrtSc`                                                  |

### 6.2 Fn number row — RGB controls (set once, forget)

`` Fn+` `` = `RGB_TOG` · 1 `RGB_MOD` · 2 `RGB_RMOD` · 3 `RGB_HUI` · 4 `RGB_SAI` · 5 `RGB_VAI` · 6 `RGB_VAD` · 7 `RGB_SPI` · 8 `RGB_SPD` · 9 = side-LED toggle (custom).

### 6.3 Fn — navigation & system

| Key          | Action                     | Mac / Linux                        |
| ------------ | -------------------------- | ---------------------------------- |
| Fn+← / →     | Word left / right          | `⌥←`/`⌥→` · `Ctrl+←`/`Ctrl+→`      |
| Fn+↑ / ↓     | Document top / bottom      | `⌘↑`/`⌘↓` · `Ctrl+Home`/`Ctrl+End` |
| Fn+End       | Home                       | `KC_HOME`                          |
| Fn+B         | **Bootloader** (`QK_BOOT`) | "B for Boot"; safe, out-of-the-way |
| Fn+N         | NKRO toggle                | `NK_TOGG`                          |
| Fn+knob push | Cycle knob mode            | scroll ↔ undo/redo (custom)        |

**Bootloader — three independent ways in** (so the layout can never be lost again): `Fn+B` · **hold Esc while plugging in** (bootmagic) · on-PCB reset button.

---

## 7. The modal knob

| Context                              | Turn                                         | Push                              |
| ------------------------------------ | -------------------------------------------- | --------------------------------- |
| Base                                 | Volume ±                                     | Play / Pause                      |
| Fn held, mode = **Scroll** (default) | Mouse wheel ↑/↓ (terminal scrollback, pages) | `Fn+push` → switch to Undo mode   |
| Fn held, mode = **Undo/Redo**        | OS-aware Undo / Redo                         | `Fn+push` → switch to Scroll mode |

- Scroll requires `MOUSEKEY_ENABLE`, sent as `MS_WHLU` / `MS_WHLD` via `tap_code16` (the current keycode names).
- Undo/Redo OS-aware: Mac `⌘Z` / `⌘⇧Z`, Linux `Ctrl+Z` / `Ctrl+Shift+Z`.
- Mode persists in a static var; only meaningful while Fn is held. Reflected live on the side strips (§8).

---

## 8. RGB / lighting

### 8.1 Per-key

- **Effect:** `RGB_MATRIX_SOLID_REACTIVE_SIMPLE` — dark base, **salmon bloom on keypress**, fades. (Chosen over literal `TYPING_HEATMAP`, which can't be recolored; feel matches the approved preview.)
- **Hot color:** salmon ≈ `RGB(255, 120, 90)`.
- **Brightness:** 64 % ⇒ default matrix value ≈ 163/255.
- **Sleep:** `RGB_MATRIX_SLEEP` — off when the Mac suspends USB.
- **Idle:** `RGB_MATRIX_TIMEOUT` = 8 min → fade off; any keypress wakes it.

### 8.2 Side strips — a functional readout (state machine)

Driven in `rgb_matrix_indicators_advanced_user`, overriding the GMMK Pro edge-LED indices each frame (never latched — this is the structural fix for the old "caps stuck lit" bug). Priority high → low:

| State                                     | Both side strips                        |
| ----------------------------------------- | --------------------------------------- |
| USB suspended / idle-timed-out            | Off                                     |
| **Caps Lock ON** (real)                   | **Pulsing red** `RGB(255,30,30)` (sine) |
| **FN layer active** — knob mode Scroll    | **Teal** `RGB(60,200,200)`              |
| **FN layer active** — knob mode Undo/Redo | **Amber** `RGB(255,170,40)`             |
| Base                                      | **Salmon** `RGB(255,120,90)`            |

One indicator, three facts: which layer you're on, which knob mode is live, and the caps safety net. Colors are trivially tunable in source.

---

## 9. Custom keycodes (enum)

| Keycode                | Purpose                                                  |
| ---------------------- | -------------------------------------------------------- |
| `OS_SHOT_CLIP`         | Screenshot region → clipboard (OS-aware) — base Shot key |
| `OS_SHOT_FILE`         | Screenshot region → file (OS-aware) — Fn+Shot            |
| `OS_MCTL`              | Mission Control / overview (OS-aware)                    |
| `OS_SPOT`              | Spotlight / search (OS-aware)                            |
| `OS_UNDO` / `OS_REDO`  | (used by knob) OS-aware undo/redo                        |
| `KNOB_MODE`            | Cycle knob mode (Fn + knob push)                         |
| `SIDE_TOGG`            | Toggle side-strip lighting (Fn+9)                        |
| `MAC_DICT` / `MAC_DND` | Fixed combos for Dictation / DND (user binds in macOS)   |

Word-nav / doc-nav OS-aware combos are handled inline (no dedicated keycode needed) via the OS helper.

---

## 10. Build configuration

**`rules.mk`**

```
OS_DETECTION_ENABLE = yes
MOUSEKEY_ENABLE     = yes      # knob scroll
NKRO_ENABLE         = yes
# RGB_MATRIX already enabled by the gmmk/pro/ansi board
```

**`config.h`**

```
#define TAPPING_TERM 200
#define HOLD_ON_OTHER_KEY_PRESS            // snappy Fn nav-layer while chording
#define RGB_MATRIX_TIMEOUT 480000          // 8 min idle → off
#define RGB_MATRIX_SLEEP
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#define RGB_MATRIX_DEFAULT_HUE  10         // salmon-ish
#define RGB_MATRIX_DEFAULT_SAT  200
#define RGB_MATRIX_DEFAULT_VAL  163        // ~64%
#define RGB_MATRIX_KEYPRESSES              // required for SOLID_REACTIVE effects
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
```

(Exact side-LED index list taken from the board's `g_led_config` during implementation.)

---

## 11. Repository layout (SSOT)

```
.
├─ CLAUDE.md                     # project instructions / SSOT pointer (created in impl)
├─ README.md                     # what this is, how to build & flash
├─ docs/
│  ├─ DESIGN.md                  # this spec
│  └─ rgb-preview.html           # the color visualizer (kept as a design tool)
├─ firmware/gati/                # the QMK keymap — the real source
│  ├─ keymap.c
│  ├─ config.h
│  ├─ rules.mk
│  └─ readme.md
├─ scripts/
│  └─ flash.sh                   # symlink into QMK tree → compile → flash
├─ reference/                    # recovered old firmware, untouched, for provenance
│  ├─ old-firmware/  (*.elf .hex .bin .map)
│  └─ gmmk_pro_ansi_layout_mine.json
└─ .gitignore
```

**Build model:** the firmware lives here; `scripts/flash.sh` symlinks `firmware/gati/` into `~/qmk_firmware/keyboards/gmmk/pro/ansi/keymaps/gati/`, then runs `qmk compile` / `qmk flash`. The repo stays the source of truth; QMK is just the compiler.

---

## 12. Toolchain setup (macOS, M2) — to be executed in the plan

1. `brew install qmk/qmk/qmk`
2. `qmk setup` (clones `qmk_firmware`, installs the ARM toolchain)
3. Symlink the keymap, `qmk compile -kb gmmk/pro/ansi -km gati`
4. Enter bootloader (`Fn+B` or hold-Esc-on-plug-in), `qmk flash …`

---

## 13. macOS one-time settings (documented in README)

- **Keyboard → Caps Lock key → "Switch to and from …"** (enables Caps=input-source).
- **Keyboard → Dictation → Shortcut → Customize** = the `MAC_DICT` combo.
- **Shortcuts.app**: a "Set Focus / Do Not Disturb" shortcut bound to the `MAC_DND` combo.

---

## 14. Open items (resolved during implementation, not blockers)

- Exact GMMK Pro side-LED index list (read from `g_led_config`).
- Final `MAC_DICT` / `MAC_DND` combos (pick two that don't collide).
- Whether to keep `rgb-preview.html` in-repo (default: yes, as a design tool).
- Optional future: a true accumulating salmon heatmap as a custom RGB effect; Linux Caps→input-switch binding.
