# CLAUDE.md — GMMK Pro QMK

Personal QMK firmware + knowledgebase for **Gati's Glorious GMMK Pro** (75% ANSI, white),
tuned for **macOS (primary, M2 Pro 14")** and **Fedora/GNOME Linux (secondary)**. Daily use:
writing, browsing, chatting, terminal, programming, Claude Code / vibe coding. No Windows.

This repo is the **single source of truth** for the keyboard: the firmware, the design behind
it, the hardware it runs on, and how to build/flash it.

---

## Source-of-truth map — which file owns what

| Topic                                                                | Authoritative file                               |
| -------------------------------------------------------------------- | ------------------------------------------------ |
| **Design** (keymap, knob, RGB, side-LEDs, rationale)                 | [`docs/DESIGN.md`](docs/DESIGN.md)               |
| **Build steps** (toolchain → firmware → flash, task by task)         | [`docs/PLAN.md`](docs/PLAN.md)                   |
| **Hardware** (parts, specs, build mods, spare keycaps, purchase log) | [`docs/HARDWARE.md`](docs/HARDWARE.md)           |
| **macOS one-time settings** (Caps→input, Dictation, DND)             | [`docs/macos-setup.md`](docs/macos-setup.md)     |
| **Firmware source** (the actual keymap — source of truth)            | `firmware/gati/{keymap.c,config.h,rules.mk}`     |
| **Build/flash tool** (symlinks into QMK → compile/flash)             | `scripts/flash.sh`                               |
| **RGB color explorer**                                               | [`docs/rgb-preview.html`](docs/rgb-preview.html) |
| **Recovered original firmware** (provenance, not reused)             | `reference/`                                     |

**Rule:** change behavior → update `docs/DESIGN.md` first, then the firmware, then flash & verify.
Never let the docs and `firmware/gati/` disagree.

---

## Status (2026-07-01)

_Single source of truth for build status — `README.md` and `docs/DESIGN.md` point here._

- ✅ Design complete & approved (`docs/DESIGN.md`).
- ✅ Implementation plan written (`docs/PLAN.md`, 7 tasks).
- ✅ Repo initialized; original firmware recovered & preserved.
- ✅ **Built, flashed & verified on hardware (v1.0).** All 7 plan tasks complete.

---

## The keyboard at a glance

**Two layers only:** `_BASE` (0) and `_FN` (1). Enter `_FN` by holding **Caps** _or_ the **Fn** key.

**Base row of note (bottom):** `Ctrl · Opt · Cmd │ Space │ Ctrl · Opt · Fn` — ⌘ sits left of space
like the MacBook. Caps = tap Caps (macOS switches input source) / hold = `_FN`.

**Knob:** base = Volume / press Play-Pause. On `_FN`: turn = **scroll ⇄ undo/redo** (modal;
`Fn`+press cycles the mode).

**Fn F-row** mirrors the MacBook: Bri− Bri+ · Mission Control · Spotlight · Dictation · DND ·
Prev Play Next · Mute Vol− Vol+.

**RGB:** dark base, **salmon** bloom on keypress (`SOLID_REACTIVE_SIMPLE`), 64% brightness,
off on idle (8 min) & Mac sleep.

**Side strips = a readout** (priority): sleep/idle→off ▸ **Caps Lock → pulsing red** ▸
**Fn held → knob-mode color** (sage scroll / amber undo) ▸ **base → off** (per-key salmon carries typing).

Full maps and the "why" behind every choice: [`docs/DESIGN.md`](docs/DESIGN.md).

---

## Non-negotiable conventions

- **Exactly two layers.** Do not add a third.
- **Keycodes must match keycap legends.** Never map a key to something its printed cap doesn't say.
- **macOS primary, Fedora secondary, no Windows.** Actions that differ between the two branch on
  `detected_host_os()` (`is_mac()` helper); unsure → macOS.
- Pure **mainline QMK**, no VIA/Vial. The C source in this repo is the source of truth.
- The full `LAYOUT` is written once and not re-edited; behavior changes go in handler functions
  (`process_record_user`, `encoder_update_user`, `rgb_matrix_indicators_advanced_user`) + config.

---

## Build & flash

```bash
./scripts/flash.sh compile     # build only
./scripts/flash.sh flash       # build + flash (put board in bootloader first)
```

- One-time toolchain: `brew install qmk/qmk/qmk` then `qmk setup` — **not** a clean two-command install on current macOS; see **Gotchas** below (trust-gates + venv fix) and `docs/DESIGN.md` §12.
- `flash.sh` symlinks `firmware/gati/` into `~/qmk_firmware/keyboards/gmmk/pro/rev1/ansi/keymaps/gati`,
  so **this repo stays the source of truth** and QMK is just the compiler.

### Enter bootloader (any of these — you can never brick the layout)

1. **Fn + B** ("B for Boot").
2. **Hold Esc while plugging in** the cable (bootmagic) — works even with broken firmware.
3. Physical reset button on the PCB (case-open, last resort).

### Change workflow

`edit docs/DESIGN.md` → `edit firmware/gati/*` → `./scripts/flash.sh compile` → flash → verify on
hardware → `git commit`. Commit after every working change; never commit `.build/`.

---

## Gotchas (hard-won)

- **RGB / setting defaults don't apply after a flash.** EEPROM survives DFU flashing. To push a new
  `RGB_MATRIX_DEFAULT_*` or mode, bump `GATI_EECONFIG_VERSION` in `firmware/gati/keymap.c` —
  `keyboard_post_init_user` re-applies it once. Otherwise the value saved in EEPROM shadows your change.
- **Toolchain (re)install, macOS:** `brew install qmk/qmk/qmk` needs `brew trust qmk/qmk`, plus
  `brew tap osx-cross/arm && brew trust osx-cross/arm` (same for `osx-cross/avr`). If `qmk --version`
  dies on a bad Python interpreter, run `brew reinstall --build-from-source qmk`.
- **The IDE lies about `keymap.c`.** Standalone clang shows false `undeclared identifier` errors (it
  lacks QMK's include context). `./scripts/flash.sh compile` is the only source of truth.
- **QMK renames keycodes across releases.** This build already hit `RGB_*`→`RM_*` and
  `KC_MS_WH_*`→`MS_WH*`. Before adding RGB / mouse / media keys, grep `~/qmk_firmware/quantum/keycodes.h`.
- **Board target is `gmmk/pro/rev1/ansi`** (`gmmk/pro/ansi` is an alias that resolves to it). The STM32
  ROM DFU bootloader is unerasable — the on-PCB reset button always re-enters DFU, so you can't brick it.

---

## Notes for future sessions

- The full history of _why_ each decision was made lives in `docs/DESIGN.md`. Read it before
  proposing changes — most "obvious improvements" were already considered and rejected for a reason
  (e.g. right-⌘ was declined to keep keycaps honest).
- Hardware quirk that drives the whole RGB design: **PBT non-shine-through caps + south-facing LEDs**
  → per-key color is only edge-glow; the **side strips carry the information**.
