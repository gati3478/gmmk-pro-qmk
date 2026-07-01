# QMK Tooling Overhaul — Design

**Date:** 2026-07-02
**Status:** Approved (design), pending spec review
**Scope:** Build/flash tooling and reproducibility for the GMMK Pro QMK firmware. **No firmware behavior changes.**

---

## Problem

The firmware is robust; the tooling around it is not. Today `scripts/flash.sh` symlinks
`firmware/gati/` into an external `~/qmk_firmware` clone and shells out to `qmk compile` / `qmk flash`.
That is the entire build chain. The audit found:

- **The QMK revision is unpinned.** The firmware builds against "whatever `~/qmk_firmware` happens to
  be" — today `0.33.8` (`b1093e9`). Nothing in the repo records or enforces it. A `git pull` in that
  clone, or a fresh clone on a new machine, can silently break or change the build. CLAUDE.md already
  documents this biting twice (`RGB_*`→`RM_*`, `KC_MS_WH_*`→`MS_WH*`). **This is the one real fragility.**
- **No CI.** No automated compile-on-push to catch breakage before hardware.
- **No format/lint config** — no `.clang-format`, `.editorconfig`, or `qmk lint` in the flow.

"Modern" is constrained by the medium: QMK is a make-based embedded-C build; there is no package
manager to modernize _into_. The achievable surface is exactly four things: **pin the QMK revision**,
**CI that compiles on push**, **format/lint config**, and a **better local wrapper**.

## Goals

1. The exact QMK revision the firmware is verified against is recorded in-repo and enforceable.
2. Every push compiles the firmware in a clean, hermetic, machine-independent environment and produces
   a downloadable `.bin`.
3. `flash.sh` is a real multi-command tool (setup / doctor / format / lint / size), not just build+flash.
4. Whitespace/format conventions are captured so editors and future edits stay consistent.

## Non-goals

- No firmware behavior changes.
- No git-submodule vendoring of QMK (would flip the "external clone, symlink in" model and add 2.3 GB).
- No local Docker/Nix — native `qmk` locally; containers only in CI (flashing is host-USB anyway).
- **No CI-enforced C style.** `keymap.c` is deliberately hand-aligned (compact `case … return false;`
  lines, the `LAYOUT` grid already guarded by `// clang-format off`). CI enforces what actually
  breaks — the build — not the author's intentional layout.
- No rename of `flash.sh` (referenced across CLAUDE.md / DESIGN / PLAN / README; rename = churn, no gain).

---

## Components

### 1. `scripts/qmk.pin` — pinned revision, single source of truth

Sourceable shell fragment, consumed by both `flash.sh` and CI:

```sh
# QMK revision this firmware is built + verified against. Bump deliberately, then re-verify + reflash.
QMK_TAG=0.33.8
QMK_SHA=b1093e9da5f27253f3db692352daf5cf4ad5b98d
# Toolchain provenance (CI pins these via the qmk_cli container image):
#   qmk CLI 1.1.8 · arm-none-eabi-gcc 9.2.1
```

Format constraint: every non-comment line is `KEY=value` so the file is safe both to `source` (bash)
and to filter into `$GITHUB_ENV` (CI, via `grep -E '^[A-Z_]+='`). Comments start with `#`.

### 2. `scripts/flash.sh` — real subcommand surface

Keep the name and the existing symlink + `:leave` watchdog logic. Add a `source "$REPO/scripts/qmk.pin"`
and a `QMK_HOME` resolution (`${QMK_HOME:-$HOME/qmk_firmware}`). Subcommands:

| Command             | Behavior                                                                                                                                                                                                                                                         |
| ------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `compile`           | `pin_check` (warn on drift, non-fatal) → symlink → `qmk compile -kb gmmk/pro/ansi -km gati`.                                                                                                                                                                     |
| `flash`             | `pin_check` → symlink → existing `flash_gmmk` watchdog path (unchanged).                                                                                                                                                                                         |
| `setup`             | If `$QMK_HOME` absent → full clone at `$QMK_TAG` with submodules. If present and **clean** → `git fetch` + `checkout $QMK_SHA` + `submodule update --init --recursive`. If present and **dirty** → abort with a clear message (never discard WIP in that clone). |
| `doctor`            | `qmk doctor`.                                                                                                                                                                                                                                                    |
| `format`            | `clang-format -i firmware/gati/keymap.c firmware/gati/config.h` using the repo-root `.clang-format`. Opt-in convenience; errors with an install hint if `clang-format` is absent.                                                                                |
| `lint`              | symlink → `qmk lint -kb gmmk/pro/ansi -km gati`. Advisory; surfaces output.                                                                                                                                                                                      |
| `size`              | build → print `.bin` byte size and `% of 256 KB` STM32F303 flash budget.                                                                                                                                                                                         |
| (default / unknown) | usage listing all subcommands.                                                                                                                                                                                                                                   |

**`pin_check()`** — compares `git -C "$QMK_HOME" rev-parse HEAD` to `$QMK_SHA`. On mismatch, print a loud
warning to stderr naming both revs and pointing at `flash.sh setup`; **continue anyway** (drift may be
intentional — testing a newer QMK). If `$QMK_HOME` is missing entirely, error out and point at `setup`.
Rationale: warn-not-block keeps the tool out of your way while making silent drift impossible.

### 3. `.editorconfig` — whitespace hygiene

```ini
root = true

[*]
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true
charset = utf-8

[*.{c,h}]
indent_style = space
indent_size = 4

[*.md]
trim_trailing_whitespace = false
```

`.mk` files are intentionally space-aligned (`NKRO_ENABLE   = yes`) and are left to the global rules —
no `indent_style` override, to avoid fighting the existing alignment.

### 4. `.clang-format` — match upstream exactly

Copy `~/qmk_firmware/.clang-format` (the pinned rev's own file) into the repo root verbatim. Reusing
QMK's own config (`ColumnLimit: 0`, `IndentWidth: 4`, the `AlignConsecutive*` family) guarantees
`flash.sh format` never diverges from the tree it compiles in. Not enforced by CI (see non-goals);
present for editor integration and opt-in `format`.

### 5. `.github/workflows/build.yml` — CI

Runs on `push`, `pull_request`, and `workflow_dispatch`. Single `build` job in the official QMK
container (which pins the ARM toolchain along with the CLI):

```yaml
name: build
on: [push, pull_request, workflow_dispatch]

jobs:
  build:
    runs-on: ubuntu-latest
    container: ghcr.io/qmk/qmk_cli:latest # pin to a specific tag/digest at implementation time
    steps:
      - name: Checkout firmware repo
        uses: actions/checkout@v4
        with: { path: fw }

      - name: Load pin
        run: grep -E '^[A-Z_]+=' fw/scripts/qmk.pin >> "$GITHUB_ENV"

      - name: Clone pinned QMK
        run: |
          git clone --depth 1 --branch "$QMK_TAG" \
            --recurse-submodules --shallow-submodules \
            https://github.com/qmk/qmk_firmware qmk
          test "$(git -C qmk rev-parse HEAD)" = "$QMK_SHA" \
            || { echo "::error::QMK tag $QMK_TAG no longer resolves to $QMK_SHA"; exit 1; }

      - name: Symlink keymap
        run: ln -sfn "$PWD/fw/firmware/gati" qmk/keyboards/gmmk/pro/rev1/ansi/keymaps/gati

      - name: Compile
        working-directory: qmk
        run: qmk compile -kb gmmk/pro/ansi -km gati

      - name: Lint (advisory)
        working-directory: qmk
        run: qmk lint -kb gmmk/pro/ansi -km gati || true

      - name: Report size
        run: |
          bin=$(ls qmk/*gati*.bin)
          size=$(wc -c < "$bin")
          echo "### Firmware $size bytes ($((size * 100 / 262144))% of 256 KB)" >> "$GITHUB_STEP_SUMMARY"

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: gmmk_pro_rev1_ansi_gati
          path: qmk/*gati*.bin
          if-no-files-found: error
```

Design notes / implementation risks to resolve during the plan:

- **`$GITHUB_ENV` parsing:** the `grep -E '^[A-Z_]+='` filter drops the `#` comment lines so only
  `KEY=value` reaches the env file (a bare comment line has no `=` and would error).
- **`QMK_HOME` detection in the container:** primary approach is `working-directory: qmk` so the CLI
  auto-detects the tree from cwd. If that proves unreliable, set a job-level `env: QMK_HOME` to the
  clone path. Verify during implementation.
- **Container tag pinning:** `:latest` is a placeholder — pin to a concrete tag or digest at
  implementation so the toolchain is fully reproducible.
- **No build cache initially** (YAGNI): shallow-clone-per-run is ~1–2 min and has no cache-eviction or
  corruption edge cases. Caching the `qmk` tree keyed by `$QMK_SHA` is a documented future optimization.
- **SHA-verify step** turns a moved tag from a silent hardware surprise into a red X before any flash.

---

## Success criteria

- `scripts/qmk.pin` records `0.33.8` / `b1093e9…`; `flash.sh setup` on a fresh machine produces a
  `~/qmk_firmware` at exactly that rev.
- `flash.sh compile` still builds the identical `44692`-byte image and now warns if the checkout drifts.
- `flash.sh` responds to `setup`, `doctor`, `format`, `lint`, `size`, and prints usage for anything else.
- Pushing to GitHub triggers a green `build` check that uploads `gmmk_pro_rev1_ansi_gati.bin`.
- `.editorconfig` and `.clang-format` present at repo root; `flash.sh format` is a no-op diff on the
  current (already-conformant) source, or its changes are reviewed and accepted once.

## Verification plan

1. `flash.sh compile` → byte-identical `.bin` to current (`44692`).
2. Temporarily point `QMK_HOME` at a throwaway checkout on a different rev → confirm `pin_check` warns
   but still builds; `flash.sh setup` brings it back to the pin.
3. Each new subcommand exercised once (`doctor`, `lint`, `size`, `format --` dry check).
4. Push a branch → confirm the Actions run goes green and the artifact downloads + matches local size.

## Docs to update after implementation

- `CLAUDE.md` — Build & flash section: new subcommands, the pin file, `setup` on a new machine, CI badge.
- `docs/PLAN.md` / `docs/DESIGN.md` §12 — reference the pinned rev and CI where they describe the toolchain.
- `README.md` — CI status badge.
