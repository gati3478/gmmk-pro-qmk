# GMMK Pro — QMK Design Spec

**Date:** 2026-07-01
**Owner:** Gati
**Status:** v1.1 shipped — see `CLAUDE.md` §Status (authoritative).
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

`` Fn+` `` = `RM_TOGG` · 1 `RM_NEXT` · 2 `RM_PREV` · 3 `RM_HUEU` · 4 `RM_SATU` · 5 `RM_VALU` · 6 `RM_VALD` · 7 `RM_SPDU` · 8 `RM_SPDD` · 9 = side-LED toggle (custom). (RGB-**matrix** keycodes; mainline QMK renamed the old `RGB_*` lighting keycodes to `RM_*`.)

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

| State                                     | Both side strips                                                                                       |
| ----------------------------------------- | ------------------------------------------------------------------------------------------------------ |
| USB suspended / idle-timed-out            | Off                                                                                                    |
| **Caps Lock ON** (real)                   | **Pulsing red** `RGB(255,0,0)` (triangle)                                                              |
| **FN layer active** — knob mode Scroll    | **Sage** `RGB(100,190,110)`                                                                            |
| **FN layer active** — knob mode Undo/Redo | **Amber** `RGB(255,170,40)`                                                                            |
| Base                                      | **Off** — the strips are a pure Fn/Caps readout; per-key salmon reactive (keylight LEDs) is unaffected |

Dark at rest — the strips stay off during normal typing (the per-key salmon ripple carries that). When lit they carry three facts: that you're on the Fn layer, which knob mode is live, and the caps-lock safety net. Colors are trivially tunable in source.

**macOS caveat — the Caps-Lock (red) row is effectively a Linux-only readout.** On macOS the physical Caps is remapped to _switch input source_ (`docs/macos-setup.md`), so the host never reports caps-lock ON and the red pulse **never fires there**. On Fedora, tapping Caps toggles real caps-lock, so it lights as designed. This is intended, not a defect — the pulse is a caps-lock safety net and only has a job where caps-lock can occur; on the primary (macOS) machine this top-priority strip state is simply dormant.

---

## 9. Custom keycodes (enum)

| Keycode                   | Purpose                                                  |
| ------------------------- | -------------------------------------------------------- |
| `OS_SHOT_CLIP`            | Screenshot region → clipboard (OS-aware) — base Shot key |
| `OS_SHOT_FILE`            | Screenshot region → file (OS-aware) — Fn+Shot            |
| `OS_MCTL`                 | Mission Control / overview (OS-aware)                    |
| `OS_SPOT`                 | Spotlight / search (OS-aware)                            |
| `OS_WORD_L` / `OS_WORD_R` | Word left / right (OS-aware) — Fn+←/→                    |
| `OS_DOC_UP` / `OS_DOC_DN` | Document top / bottom (OS-aware) — Fn+↑/↓                |
| `KNOB_MODE`               | Cycle knob mode (Fn + knob push)                         |
| `SIDE_TOGG`               | Toggle side-strip lighting (Fn+9)                        |

`MAC_DICT` / `MAC_DND` (⌃⌥⌘D / ⌃⌥⌘N) are fixed-combo `#define`s dropped straight into the Fn layer, **not** enum keycodes. Undo/redo is OS-aware but handled inline in `encoder_update_user` (the knob), so it needs no dedicated keycode.

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
#define RGB_MATRIX_DEFAULT_HUE  12         // salmon-ish
#define RGB_MATRIX_DEFAULT_SAT  165
#define RGB_MATRIX_DEFAULT_VAL  163        // ~64%
#define RGB_MATRIX_KEYPRESSES              // required for SOLID_REACTIVE effects
#define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
```

(Side strips are targeted by the `LED_FLAG_UNDERGLOW` (`0x02`) flag in `g_led_config`, not a hard-coded index list. A version stamp in the user-EEPROM block (`eeconfig_*_user`) re-applies the RGB defaults once after a flash, since DFU never erases EEPROM.)

---

## 11. Repository layout (SSOT)

```
.
├─ CLAUDE.md                     # project instructions / SSOT pointer
├─ README.md                     # what this is, how to build & flash
├─ docs/
│  ├─ DESIGN.md                  # this spec
│  ├─ PLAN.md                    # task-by-task build record (v1.0)
│  ├─ HARDWARE.md                # parts, specs, mods, purchase log
│  ├─ macos-setup.md             # one-time macOS settings
│  ├─ rgb-preview.html           # the color visualizer (kept as a design tool)
│  └─ superpowers/               # spec + plan for the tooling overhaul
├─ firmware/gati/                # the QMK keymap — the real source
│  ├─ keymap.c
│  ├─ config.h
│  ├─ rules.mk
│  └─ readme.md
├─ scripts/
│  ├─ flash.sh                   # symlink into QMK tree → compile/flash/setup/…
│  └─ qmk.pin                    # pinned QMK rev + ARM toolchain (shared with CI)
├─ .github/workflows/build.yml   # CI: compile-on-push, uploads firmware artifact
├─ reference/                    # recovered old firmware, untouched, for provenance
│  ├─ old-firmware/  (*.elf .hex .bin .map)
│  └─ gmmk_pro_ansi_layout_mine.json
├─ .clang-format · .editorconfig  # C formatting + editor defaults
└─ .gitattributes · .gitignore
```

**Build model:** the firmware lives here; `scripts/flash.sh` symlinks `firmware/gati/` into `~/qmk_firmware/keyboards/gmmk/pro/rev1/ansi/keymaps/gati/` (the real rev1 path — `gmmk/pro/ansi` is only a `-kb` alias), then runs `qmk compile` / `qmk flash`. The repo stays the source of truth; QMK is just the compiler.

---

## 12. Toolchain, pinning & reproducibility (macOS, M2)

**Install (current method).** `curl -fsSL https://install.qmk.fm | sh` installs the `qmk` CLI and the
qmk_toolchains ARM compiler (gcc 15.2.0) into `~/Library/Application Support/qmk`, which the CLI
auto-prepends to PATH. Then `./scripts/flash.sh setup` clones `~/qmk_firmware` at the pinned rev.
(The older `brew install qmk/qmk/qmk` is stale — it pulls gcc 8 and cannot match CI.)

**Pinning.** `scripts/qmk.pin` records the exact QMK revision (`0.33.8` / `b1093e9`) and ARM toolchain
(`arm-none-eabi-gcc 15.2.0`, qmk_toolchains commit `a55a2b0`) — the single source of truth shared by
`flash.sh` (local) and `.github/workflows/build.yml` (CI, which pins the `qmk_cli` container by digest).
CI verifies the cloned QMK's SHA equals the pin, so a moved tag fails loudly before any hardware.

**Reproducibility — the honest caveat.** Local (macOS, Apple Silicon) and CI (Linux) use the _same_
qmk_toolchains commit and gcc 15.2.0, so they produce **functionally identical** firmware — but **not**
bit-for-bit identical. The toolchain bakes its own build-machine path into a newlib-nano `assert` string
(`host_macosARM64…` locally vs `host_linuxX64…` in CI): a +4-byte inert difference that shifts image
layout (~1741 bytes differ, **zero** of them machine code). Neither we nor QMK control this without
building both on the same host OS (a macOS CI runner — declined on cost). So the **local build is
authoritative** for what gets flashed; CI is the clean-room compile gate + a reference artifact. This
keymap embeds nothing time- or version-dependent in its `.bin` (`COMMAND`/`CONSOLE` off, no `QMK_VERSION`
refs), so local rebuilds are themselves deterministic (byte-identical run to run).

**Build/flash.** `./scripts/flash.sh compile` → `flash` (enter bootloader: `Fn+B` or hold-Esc-on-plug-in).

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
