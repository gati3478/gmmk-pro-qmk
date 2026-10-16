# GMMK Pro — QMK firmware & knowledgebase

[![build](https://github.com/gati3478/gmmk-pro-qmk/actions/workflows/build.yml/badge.svg)](https://github.com/gati3478/gmmk-pro-qmk/actions/workflows/build.yml)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL--2.0--or--later-blue.svg)](LICENSE)

**A two-layer, macOS-first QMK keymap for the Glorious GMMK Pro (75% ANSI) —
built like production software: pinned reproducible builds, CI, and
hardware-verified releases.**

Daily driver for writing, terminal work, programming, and Claude Code on macOS
(primary) and Fedora/GNOME (secondary). No Windows. This repo is the single
source of truth for the board — firmware, design rationale, hardware log, and
tooling.

## The keymap in 10 seconds

```text
Caps       tap = Caps Lock (macOS: switch input source) · hold = Fn layer
Knob       turn = Volume · press = Play/Pause — with Fn: scroll ⇄ undo/redo (Fn+press cycles)
Fn F-row   the MacBook media row: Bri− Bri+ · Mission Ctl · Spotlight · Dictation · DND
           · Prev Play Next · Mute Vol− Vol+
Bottom     Ctrl · Opt · Cmd │ Space │ Ctrl · Opt · Fn     (⌘ left of Space, like a MacBook)
Side LEDs  a readout: pulsing red = Caps Lock · sage/amber = knob mode · off at rest
Per-key    dark base, salmon bloom on each keypress; off after 8 min idle & on Mac sleep
```

Everything else is standard ANSI. Full maps and the "why" behind every choice:
[`docs/DESIGN.md`](docs/DESIGN.md).

## Highlights

- **Exactly two layers, honest keycaps.** Every binding earns its place, and a
  keycode never disagrees with the printed cap.
- **Side strips carry the information.** PBT non-shine-through caps over
  south-facing LEDs mean per-key RGB reads as edge glow only — so state
  (layer, knob mode, caps lock) lives on the side strips instead.
- **OS-aware.** Screenshot, Mission Control / overview, word- and document-nav,
  undo/redo resolve per host at runtime; unknown hosts default to macOS.
- **Reproducible.** QMK revision + ARM toolchain pinned in
  [`scripts/qmk.pin`](scripts/qmk.pin); CI builds in a digest-pinned container
  from a SHA-verified QMK clone with SHA-pinned actions.
- **Unbrickable.** The STM32 ROM DFU bootloader is unerasable — `Fn+B` (held 1 s),
  Esc-while-plugging-in, or the PCB reset button always recover.

## Map

| Read this for…                                          | File                                             |
| ------------------------------------------------------- | ------------------------------------------------ |
| Project brief, conventions, quickstart (**start here**) | [`CLAUDE.md`](CLAUDE.md)                         |
| Full design — keymap, knob, RGB, side-LEDs + rationale  | [`docs/DESIGN.md`](docs/DESIGN.md)               |
| Hardware — parts, specs, mods, spare keycaps, purchases | [`docs/HARDWARE.md`](docs/HARDWARE.md)           |
| v1.0 build record — task by task (history)              | [`docs/PLAN.md`](docs/PLAN.md)                   |
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
**Enter bootloader:** hold **Fn+B** for 1 s, or hold **Esc** while plugging in the cable.

## Forking this for your own board

The keymap is plain mainline QMK — no VIA/Vial. Start from
`firmware/gati/{keymap.c,config.h,rules.mk}`, adjust the `LAYOUT`, the
`encoder_map`, and the handler functions (`process_record_user`,
`rgb_matrix_indicators_advanced_user`), and point `scripts/flash.sh` at your
keyboard target. `docs/DESIGN.md` explains why each piece is the way it is —
including the ideas that were considered and rejected.

## Status

**v1.2** — hardware-verified: v1.1 firmware behavior plus the reproducible
build chain and public-release polish. Live status: [`CLAUDE.md`](CLAUDE.md)
§Status · versions: [`CHANGELOG.md`](CHANGELOG.md) · flashable `.bin`:
[Releases](https://github.com/gati3478/gmmk-pro-qmk/releases).

## License

[GPL-2.0-or-later](LICENSE) — the keymap is a derivative of
[QMK](https://github.com/qmk/qmk_firmware), which is GPL-licensed.
Documentation and tooling in this repo carry the same license for simplicity.
