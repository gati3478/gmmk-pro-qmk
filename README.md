# GMMK Pro — QMK firmware & knowledgebase

[![build](https://github.com/gati3478/gmmk-pro-qmk/actions/workflows/build.yml/badge.svg)](https://github.com/gati3478/gmmk-pro-qmk/actions/workflows/build.yml)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL--2.0--or--later-blue.svg)](LICENSE)

**A two-layer, macOS-first QMK keymap for the Glorious GMMK Pro (75% ANSI) —
with the full engineering treatment: pinned reproducible builds, CI,
hardware-verified releases, and design docs that record the "why" behind every
binding.**

Tuned for macOS (primary) and Fedora/GNOME (secondary) — writing, browsing,
terminal, programming, and Claude Code. No Windows. This repo is the single
source of truth for the keyboard: firmware, design rationale, hardware build
log, and tooling.

## Highlights

- **Exactly two layers.** `_BASE` and `_FN` (hold Caps _or_ Fn) — every binding
  earns its place, and keycodes always match the printed keycaps.
- **Modal knob.** Volume / Play-Pause on base; on Fn the turn becomes
  **scroll ⇄ undo/redo** (Fn+push cycles the mode).
- **Side strips as a readout.** Pulsing red = Caps Lock, sage = knob-scroll,
  amber = knob-undo, off at rest — the per-key salmon reactive glow carries
  typing. (PBT non-shine-through caps + south-facing LEDs make the strips the
  information surface; see `docs/DESIGN.md`.)
- **OS-aware keys.** Screenshot, Mission Control / overview, word- and
  document-nav, undo/redo resolve per-host via QMK's OS detection; unknown
  hosts default to macOS.
- **Reproducible builds.** QMK revision + ARM toolchain pinned in
  [`scripts/qmk.pin`](scripts/qmk.pin); CI compiles in a digest-pinned
  container with SHA-verified sources and SHA-pinned actions, and uploads the
  firmware artifact.
- **Unbrickable.** The STM32 ROM DFU bootloader is unerasable — `Fn+B`,
  Esc-while-plugging-in, or the PCB reset button always recover.

## Map

| Read this for…                                          | File                                             |
| ------------------------------------------------------- | ------------------------------------------------ |
| Project brief, conventions, quickstart (**start here**) | [`CLAUDE.md`](CLAUDE.md)                         |
| Full design — keymap, knob, RGB, side-LEDs + rationale  | [`docs/DESIGN.md`](docs/DESIGN.md)               |
| Hardware — parts, specs, mods, spare keycaps, purchases | [`docs/HARDWARE.md`](docs/HARDWARE.md)           |
| Build plan — toolchain → firmware → flash, task by task | [`docs/PLAN.md`](docs/PLAN.md)                   |
| Release history                                         | [`CHANGELOG.md`](CHANGELOG.md)                   |
| macOS one-time settings                                 | [`docs/macos-setup.md`](docs/macos-setup.md)     |
| RGB color explorer (open in a browser)                  | [`docs/rgb-preview.html`](docs/rgb-preview.html) |
| Recovered original firmware (provenance, not reused)    | [`reference/`](reference/)                       |

## Build & flash

```bash
./scripts/flash.sh compile     # build only  (setup|size|lint|format|doctor also available)
./scripts/flash.sh flash       # build + flash (enter bootloader first)
```

QMK rev + ARM toolchain are pinned in [`scripts/qmk.pin`](scripts/qmk.pin) and built in CI on every firmware push.
One-time toolchain (macOS): `curl -fsSL https://install.qmk.fm | sh`, then `./scripts/flash.sh setup`.
**Enter bootloader:** press **Fn+B**, or hold **Esc** while plugging in the cable.

## Forking this for your own board

The keymap is plain mainline QMK — no VIA/Vial. Start from
`firmware/gati/{keymap.c,config.h,rules.mk}`, adjust the `LAYOUT` and the
handler functions (`process_record_user`, `encoder_update_user`,
`rgb_matrix_indicators_advanced_user`), and point `scripts/flash.sh` at your
keyboard target. `docs/DESIGN.md` explains why each piece is the way it is —
including the ideas that were considered and rejected.

## Status

**v1.2** — flashed & verified on hardware. Firmware behavior is v1.1
(post-audit hardening); v1.2 adds the pinned/reproducible build chain, CI, and
the public-release polish. Authoritative status: [`CLAUDE.md`](CLAUDE.md)
§Status · versions: [`CHANGELOG.md`](CHANGELOG.md) ·
[Releases](https://github.com/gati3478/gmmk-pro-qmk/releases) ship the
hardware-verified `.bin`.

## License

[GPL-2.0-or-later](LICENSE) — the keymap is a derivative of
[QMK](https://github.com/qmk/qmk_firmware), which is GPL-licensed.
Documentation and tooling in this repo carry the same license for simplicity.
