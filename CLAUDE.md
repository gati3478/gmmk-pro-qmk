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
| **v1.0 build record** (history; toolchain steps superseded below)    | [`docs/PLAN.md`](docs/PLAN.md)                   |
| **Hardware** (parts, specs, build mods, spare keycaps, purchase log) | [`docs/HARDWARE.md`](docs/HARDWARE.md)           |
| **macOS one-time settings** (Caps→input, Dictation, DND)             | [`docs/macos-setup.md`](docs/macos-setup.md)     |
| **Firmware source** (the actual keymap — source of truth)            | `firmware/gati/{keymap.c,config.h,rules.mk}`     |
| **Build/flash tool** (symlinks into QMK → compile/flash/setup/…)     | `scripts/flash.sh`                               |
| **Build pin** (QMK rev + ARM toolchain — pinned source of truth)     | `scripts/qmk.pin`                                |
| **CI** (compile + shellcheck on push, uploads firmware artifact)     | `.github/workflows/build.yml`                    |
| **Tooling-overhaul spec & plan** (history of the pin/CI work)        | `docs/superpowers/{specs,plans}/`                |
| **Release history** (per-version changes, `vX.Y` milestones)         | [`CHANGELOG.md`](CHANGELOG.md)                   |
| **RGB color explorer**                                               | [`docs/rgb-preview.html`](docs/rgb-preview.html) |
| **Recovered original firmware** (provenance, not reused)             | `reference/`                                     |

**Rule:** change behavior → update `docs/DESIGN.md` first, then the firmware, then flash & verify.
Never let the docs and `firmware/gati/` disagree.

---

## Status (2026-10-16)

_Single source of truth for build status — `README.md` and `docs/DESIGN.md` point here._

**Current: v1.2 released. v1.3 (audit fixes) is on `main`, compiled clean — awaiting flash + hardware verification before the `v1.3` release.**

- ✅ **v1.0** — designed (`docs/DESIGN.md`), planned (`docs/PLAN.md`, 7 tasks), built, flashed &
  verified; original firmware recovered (`reference/`).
- ✅ **v1.1** — post-audit hardening. Adversarial audit passed (no brick/hang/corruption paths);
  encoder Volume-blip "fix" during a Caps-hold (a no-op — see v1.3) + 32-bit caps-pulse timer; `flash.sh` auto-kills
  dfu-util wedged on `:leave` (Gotchas); caps-red strip documented macOS-dormant (`docs/DESIGN.md` §8.2).
- ✅ **Tooling overhaul** — QMK rev + ARM toolchain pinned (`scripts/qmk.pin`); digest-pinned CI
  (`.github/workflows/build.yml`); `flash.sh` gained `setup/doctor/lint/format/size` + pin-drift
  warning. Firmware behavior unchanged from v1.1 (`docs/DESIGN.md` §12).
- ✅ **v1.2 — public release** (bundles the tooling overhaul above): LICENSE (GPL-2.0-or-later,
  QMK derivative) + SPDX headers; `CHANGELOG.md`; README overhaul; repo public with GitHub
  releases v1.0–v1.2 (v1.2 ships the hardware-verified `.bin`). Firmware behavior unchanged.
- 🔧 **v1.3 — audit fixes** (2026-10-16, unverified on hardware): rolled Caps taps stay taps, sloppy nav chords stay
  Fn, encoder map (real knob/Caps fix), 1 s Fn+B, strips follow brightness, QMK 0.34.7; tooling hardening.

_Milestone convention: new entries get one bullet (≤3 lines); details live with their owning file._

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
  (`process_record_user`, `rgb_matrix_indicators_advanced_user`, `housekeeping_task_user`), the
  `encoder_map`, and config.
- **`reference/` is frozen provenance.** Never edit its contents. A PreToolUse hook
  (`.claude/hooks/guard-reference.sh`) blocks Edit/Write there (not Bash); CI's `provenance` job fails any push
  that changes the `reference/` tree.

---

## Build & flash

```bash
./scripts/flash.sh compile     # build only (warns on QMK-rev or compiler drift from the pin)
./scripts/flash.sh flash       # build + flash (bootloader first: hold Fn+B 1 s)
./scripts/flash.sh setup       # clone/checkout ~/qmk_firmware to the pinned QMK rev (new machine / bump)
./scripts/flash.sh size        # build + report firmware size vs the 256 KB flash budget
./scripts/flash.sh lint        # qmk lint (advisory)
./scripts/flash.sh format      # clang-format the C sources (--check = dry run; fails on the hand-aligned blocks — not a gate)
./scripts/flash.sh doctor      # qmk doctor (toolchain preflight)
```

- **Pinned build.** `scripts/qmk.pin` is the single source of truth for the QMK revision (`0.34.7`)
  and the ARM toolchain (`QMK_GCC=15.2.0`, qmk_toolchains `a55a2b0`), shared by `flash.sh`
  and CI. `compile`/`flash`/`lint`/`size` warn if `~/qmk_firmware` has drifted off the pinned rev;
  builds warn (CI fails) when the gcc version stamped in the `.elf` isn't `QMK_GCC`.
- **CI.** `.github/workflows/build.yml` compiles every firmware-relevant push (doc-only pushes are
  skipped) in the digest-pinned `qmk_cli` container and uploads `gmmk_pro_rev1_ansi_gati.bin`; a
  SHA-verify step fails loudly if the tag moves; shellcheck and `provenance` jobs run alongside.
  Post-push checks only — `main` has no branch protection.
- **One-time toolchain (macOS, Apple Silicon):** `curl -fsSL https://install.qmk.fm | sh`, then
  `./scripts/flash.sh setup`. Details (install dir, staying pinned, why not brew): **Gotchas**.
- `flash.sh` symlinks `firmware/gati/` into `~/qmk_firmware/keyboards/gmmk/pro/rev1/ansi/keymaps/gati`,
  so **this repo stays the source of truth** and QMK is just the compiler.
- **Output is deterministic.** The `.bin` lands at `~/qmk_firmware/gmmk_pro_rev1_ansi_gati.bin`, byte-identical
  run to run. A change claimed behavior-neutral must reproduce the previous build's sha256 at the
  same pin (`shasum -a 256 <bin>`; v1.2 @ 0.33.8 = `719fd234…`).

### Enter bootloader (any of these — you can never brick the layout)

1. **Hold Fn + B for 1 s** ("B for Boot"; the hold stops a stray chord from triggering it).
2. **Hold Esc while plugging in** the cable (bootmagic) — works even with a broken keymap
   (needs firmware that boots; a corrupt image → option 3).
3. Physical reset button on the PCB (case-open, last resort).

### Change workflow

`edit docs/DESIGN.md` → `edit firmware/gati/*` → `./scripts/flash.sh compile` → flash → verify on
hardware → `git commit` → `git push`. Commit after every working change; never commit `.build/`.
Behavior changes also get a `CHANGELOG.md` entry under **[Unreleased]**.
No automated tests: compile + lint prove only that it builds; **verified** means flashed and exercised on the board.

**Release:** `CHANGELOG.md` `[Unreleased]` → `[X.Y] - date` (+ compare link) → tag `vX.Y` →
`gh release create vX.Y ~/qmk_firmware/gmmk_pro_rev1_ansi_gati.bin` — the hardware-verified local build,
not CI's artifact (`docs/DESIGN.md` §12).
Lives at **`github.com/gati3478/gmmk-pro-qmk`** (public; `origin` is SSH → `git push` just works).

---

## Gotchas (hard-won)

- **RGB / setting defaults don't apply after a flash.** EEPROM survives DFU flashing. To push a new
  `RGB_MATRIX_DEFAULT_*` or mode, bump `GATI_EECONFIG_VERSION` in `firmware/gati/keymap.c` —
  `keyboard_post_init_user` re-applies it once. Otherwise the value saved in EEPROM shadows your change.
- **Toolchain install, macOS (current method):** `curl -fsSL https://install.qmk.fm | sh` installs the
  `qmk` CLI + the qmk_toolchains ARM compiler (gcc 15.2.0) into `~/Library/Application Support/qmk`, which
  the `qmk` CLI auto-prepends to PATH (no config, shadows any old `/usr/local/bin` gcc). **Don't** use
  `brew install qmk/qmk/qmk` — the tap is stale, pulling gcc 8 via `osx-cross`, and can't match CI.
  `install.qmk.fm` always fetches the _latest_ toolchain; to stay matched to `scripts/qmk.pin`, extract the
  `host_macosARM64` asset of qmk_toolchains `v15.2.0-1` into that dir manually if QMK ships a newer one.
- **Bumping QMK or the toolchain:** edit `scripts/qmk.pin`, `./scripts/flash.sh setup`, recompile,
  re-verify on hardware, reflash. CI's SHA-verify guards against a tag silently moving underneath you.
- **`process_record_user` sees a layer-tap press only after it settles** (timeout, interrupting key,
  or release), never while it's pending. Raw-press logic belongs in `pre_process_record_user`.
  This is why v1.1's `caps_held` knob fix was a no-op; the encoder map (detents = key events) replaced it.
- **The IDE lies about `keymap.c`.** Standalone clang shows false `undeclared identifier` errors (it
  lacks QMK's include context). `./scripts/flash.sh compile` is the only source of truth.
- **QMK renames keycodes across releases.** This build already hit `RGB_*`→`RM_*` and
  `KC_MS_WH_*`→`MS_WH*`. Before adding RGB / mouse / media keys, grep `~/qmk_firmware/quantum/keycodes.h`.
- **Board target is `gmmk/pro/rev1/ansi`** (`gmmk/pro/ansi` is an alias that resolves to it). The STM32
  ROM DFU bootloader is unerasable — the on-PCB reset button always re-enters DFU, so you can't brick it.
- **`dfu-util` intermittently wedges on `:leave` (macOS).** After a successful write, dfu-util issues the
  DFU_DETACH reset; the board reboots into the new firmware but dfu-util **sometimes** never gets its ack and
  blocks forever (once cost a 57-min zombie flash; other flashes exit cleanly). The write is already done — the
  board is fine. `scripts/flash.sh` handles
  this: it watches for dfu-util's `File downloaded successfully` line, then kills a still-wedged dfu-util
  after `FLASH_LEAVE_GRACE` (default 10s) and reports success. A blanket `timeout` can't be used — the
  pre-write "waiting for bootloader" phase is legitimately open-ended; only that marker separates the phases.
  If `flash.sh` ever kills _before_ that line, it's a real flash failure, not the `:leave` quirk.

---

## Notes for future sessions

- The full history of _why_ each decision was made lives in `docs/DESIGN.md`. Read it before
  proposing changes — most "obvious improvements" were already considered and rejected for a reason
  (e.g. right-⌘ was declined to keep keycaps honest).
- Hardware quirk that drives the whole RGB design: **PBT non-shine-through caps + south-facing LEDs**
  → per-key color is only edge-glow; the **side strips carry the information**.
