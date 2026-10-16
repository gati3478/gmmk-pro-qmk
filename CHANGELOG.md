# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions are repo milestones (`vMAJOR.MINOR`), not strict SemVer — this is
firmware + docs, not a library. Every released firmware is flashed and
verified on hardware before it's tagged.

## [Unreleased]

Repo audit (2026-10-16). Built and compiled clean; **not yet verified on
hardware**.

### Fixed

- A Caps tap rolled into a typing key (Caps released first) is a Caps tap
  again. `HOLD_ON_OTHER_KEY_PRESS` resolved every roll as Fn, so a fast
  language-switch-then-type could enter the bootloader (Caps→B), toggle NKRO
  (Caps→N), change persisted RGB settings (Caps→`/1–8), or silently drop the
  switch. Now `PERMISSIVE_HOLD`: only a key tapped inside the hold is Fn.
- Sloppy Fn chords via Caps work: a nav/media/knob key (F-row, Shot, knob,
  End, arrows) pressed during a Caps press makes it Fn at once, even if Caps
  is released first. Trade-off: a language switch rolled straight into one of
  these keys reads as Fn too — the two are the same key events.
- Tapping Caps and holding it again within 200 ms gives Fn, not a repeated
  Caps tap (`QUICK_TAP_TERM 0`).
- Turning the knob in the first 200 ms of a Caps hold acts on Fn — no Volume
  blip, no stray Caps tap. The knob is now an encoder map, so each detent is a
  key event that settles the layer-tap. Replaces v1.1's `caps_held`, which
  never worked (`process_record_user` sees a layer-tap press only after it
  settles).
- Side strips toggled off (Fn+9) stay dark under any RGB effect; before, an
  animated effect showed through.
- Side strips follow the global brightness (Fn+5/6); before, they ignored it
  and ran brighter than the keys.
- `flash.sh`: Ctrl-C during `flash` now kills the qmk/dfu-util tree (before,
  it kept running and flashed the next Fn+B unsupervised); runs qmk from the
  pinned checkout; accepts git-worktree checkouts; rejects unknown `format`
  options instead of rewriting files.

### Changed

- Bootloader key (Fn+B) needs a 1 s hold.
- QMK 0.33.8 → 0.34.7 (no `gmmk/pro` changes; picks up the reactive-effect
  last-hit tracker fix, qmk/qmk_firmware#26306).
- The toolchain pin is checked: `QMK_GCC` in `scripts/qmk.pin`; builds warn
  and CI fails when the `.elf`'s gcc differs.
- CI actions moved to Node 24 (checkout v7.0.1, upload-artifact v7.0.2).

### Added

- Claude Code guard hook (`.claude/hooks/guard-reference.sh`): Edit/Write
  under this repo's `reference/` is blocked — path-normalized (`..`, `//`),
  case-insensitive, failing closed when `jq` is missing.
- CI `provenance` job: fails if the `reference/` git tree changes.

### Docs

- Corrected stale lines across `DESIGN.md`, `README.md`, `CLAUDE.md`,
  `scripts/qmk.pin`, and this file.

## [1.2] - 2026-07-02

Tooling overhaul + public release. **No firmware behavior changes** — same
behavior as v1.1, rebuilt on the modern toolchain and re-verified on hardware
(typing / knob / RGB).

### Added

- `scripts/qmk.pin` — QMK revision (`0.33.8`) **and** ARM toolchain
  (`arm-none-eabi-gcc 15.2.0`, qmk_toolchains `a55a2b0`) pinned; the single
  source of truth shared by `flash.sh` and CI.
- CI (`.github/workflows/build.yml`): compiles every firmware-relevant push in
  a digest-pinned `qmk_cli` container, SHA-verifies the QMK tag, uploads the
  firmware artifact; a shellcheck job checks `flash.sh`; GitHub Actions are
  SHA-pinned; doc-only pushes are skipped.
- `flash.sh` subcommands: `setup` / `doctor` / `lint` / `format` / `size`,
  plus a pin-drift warning on every build.
- Public-release polish: `LICENSE` (GPL-2.0-or-later — this is a QMK
  derivative), this changelog, README overhaul, SPDX headers on the firmware
  sources.

### Changed

- Local toolchain modernized `gcc` 9.2.1 → 15.2.0 to match CI. Local (macOS)
  and CI (Linux) builds are functionally identical, differing only by an inert
  newlib assert-path string — see `docs/DESIGN.md` §12.

## [1.1] - 2026-07-01

Post-audit hardening. An adversarial audit (cross-checked against QMK source)
passed: no brick / hang / corruption paths.

### Fixed

- Encoder no longer blips Volume when turned during the Caps-hold tapping
  window (raw `caps_held` tracking bypasses the layer-tap state machine).
  **Correction:** this never worked — fixed for real in [Unreleased].
- Caps-lock side-strip pulse timer widened to 32-bit, killing a ~65 s wrap
  hiccup.

### Added

- `flash.sh`: auto-kill for dfu-util wedged on the post-write `:leave` USB
  reset (macOS quirk — the write is already complete; once cost a 57-minute
  zombie flash).

### Docs

- Caps-red side strip documented as macOS-dormant (`docs/DESIGN.md` §8.2) —
  macOS remaps Caps to input-source switching, so the host never reports
  caps-lock there.

## [1.0] - 2026-07-01

First working build — flashed & verified on hardware.

### Added

- Two-layer keymap (`_BASE` + `_FN` via Caps-hold or the Fn key): macOS-first,
  Fedora-aware (`detected_host_os()`), MacBook-mirroring Fn F-row, OS-aware
  screenshot / word-nav / document-nav keys.
- Modal rotary knob: Volume / Play-Pause on base; **scroll ⇄ undo/redo** on Fn
  (Fn+push cycles the mode).
- RGB: salmon per-key reactive (`SOLID_REACTIVE_SIMPLE`, 64% brightness),
  8-minute idle timeout, off on USB suspend.
- Side-strip readout: pulsing red = Caps Lock, sage = knob-scroll,
  amber = knob-undo, off at rest.
- EEPROM version stamp (`GATI_EECONFIG_VERSION`) re-applies RGB defaults
  exactly once per bump — DFU flashing never erases EEPROM.
- `scripts/flash.sh` (symlinks the keymap into a QMK checkout, builds,
  flashes); the original pre-repo QMK build (Windows-era) recovered and preserved under
  `reference/` for provenance.

[Unreleased]: https://github.com/gati3478/gmmk-pro-qmk/compare/v1.2...HEAD
[1.2]: https://github.com/gati3478/gmmk-pro-qmk/compare/v1.1...v1.2
[1.1]: https://github.com/gati3478/gmmk-pro-qmk/compare/v1.0...v1.1
[1.0]: https://github.com/gati3478/gmmk-pro-qmk/releases/tag/v1.0
