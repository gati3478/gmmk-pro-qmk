# GMMK Pro — QMK (macOS / Linux)

[![build](https://github.com/gati3478/gmmk-pro-qmk/actions/workflows/build.yml/badge.svg)](https://github.com/gati3478/gmmk-pro-qmk/actions/workflows/build.yml)

Personal QMK firmware **and** knowledgebase for a Glorious **GMMK Pro** (75% ANSI, white),
tuned for **macOS (primary)** and **Fedora/GNOME Linux (secondary)** — writing, browsing,
terminal, programming, and Claude Code. No Windows. This repo is the single source of truth
for the keyboard.

## Map

| Read this for…                                          | File                                             |
| ------------------------------------------------------- | ------------------------------------------------ |
| Project brief, conventions, quickstart (**start here**) | [`CLAUDE.md`](CLAUDE.md)                         |
| Full design — keymap, knob, RGB, side-LEDs + rationale  | [`docs/DESIGN.md`](docs/DESIGN.md)               |
| Hardware — parts, specs, mods, spare keycaps, purchases | [`docs/HARDWARE.md`](docs/HARDWARE.md)           |
| Build plan — toolchain → firmware → flash, task by task | [`docs/PLAN.md`](docs/PLAN.md)                   |
| macOS one-time settings                                 | [`docs/macos-setup.md`](docs/macos-setup.md)     |
| RGB color explorer (open in a browser)                  | [`docs/rgb-preview.html`](docs/rgb-preview.html) |
| Recovered original firmware (provenance, not reused)    | [`reference/`](reference/)                       |

## Build & flash

```bash
./scripts/flash.sh compile     # build only  (setup|size|lint|format|doctor also available)
./scripts/flash.sh flash       # build + flash (enter bootloader first)
```

QMK rev + ARM toolchain are pinned in [`scripts/qmk.pin`](scripts/qmk.pin) and built in CI on every push.
One-time toolchain (macOS): `curl -fsSL https://install.qmk.fm | sh`, then `./scripts/flash.sh setup`.
**Enter bootloader:** press **Fn+B**, or hold **Esc** while plugging in the cable.

## Status

**v1.1** — built, flashed & verified on hardware; post-audit hardening. Branch `tooling-overhaul` adds a
pinned QMK + modern toolchain (gcc 15.2.0), push-triggered CI, and an expanded `flash.sh`. Authoritative
status: [`CLAUDE.md`](CLAUDE.md) §Status.
