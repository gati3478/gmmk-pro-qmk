# GMMK Pro — QMK (macOS / Linux)

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
./scripts/flash.sh compile     # build only
./scripts/flash.sh flash       # build + flash (enter bootloader first)
```

One-time toolchain: `brew install qmk/qmk/qmk` then `qmk setup`.
**Enter bootloader:** press **Fn+B**, or hold **Esc** while plugging in the cable.

## Status

**v1.1** — built, flashed & verified on hardware; post-audit hardening. Authoritative status: [`CLAUDE.md`](CLAUDE.md) §Status.
