# QMK Tooling Overhaul Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the GMMK Pro QMK build reproducible and modern — pin the QMK revision, add push-triggered CI that compiles and uploads the firmware, and turn `flash.sh` into a real multi-command tool — without changing any firmware behavior.

**Architecture:** The firmware still lives in `firmware/gati/` and is symlinked into an external `~/qmk_firmware` clone; QMK stays the compiler. A new `scripts/qmk.pin` records the exact QMK revision and is the single source of truth shared by `flash.sh` (local) and GitHub Actions (CI). `flash.sh` gains subcommands and a pin-drift warning; CI reproduces the local build in QMK's official container. **Execute on a branch `tooling-overhaul`** so the CI workflow can be validated on a push before it lands on `main`.

**Tech Stack:** Bash, QMK CLI 1.1.8, GitHub Actions, `ghcr.io/qmk/qmk_cli` container, clang-format, EditorConfig.

## Global Constraints

- **No firmware behavior changes.** `firmware/gati/{keymap.c,config.h,rules.mk}` are not edited by this work.
- **`flash.sh compile` must stay byte-identical** — the produced `gmmk_pro_rev1_ansi_gati.bin` is `44692` bytes today; a regression test asserts the SHA is unchanged.
- **Pinned revision, exact:** `QMK_TAG=0.33.8`, `QMK_SHA=b1093e9da5f27253f3db692352daf5cf4ad5b98d`.
- **Keep the name `flash.sh`** (referenced across CLAUDE.md / DESIGN / PLAN / README).
- **CI does not enforce C style** — it enforces the build + `qmk lint` (advisory). `.clang-format`/`format` are opt-in and never run against the hand-aligned sources as part of this work.
- **Native local, container only in CI.** No local Docker/Nix.
- Board: `-kb gmmk/pro/ansi -km gati` (alias resolves to `rev1/ansi`). Flash budget: STM32F303 = 256 KB.

## Before you start

Work on a dedicated branch so CI can be validated on a push before it reaches `main`:

```bash
git -C "/Users/gati3478/Development/Keyboard GMMK Pro" switch -c tooling-overhaul
```

All task commits land on `tooling-overhaul`; the final verification merges it back via
`superpowers:finishing-a-development-branch`.

---

### Task 1: Record the pinned QMK revision

**Files:**

- Create: `scripts/qmk.pin`

**Interfaces:**

- Produces: shell variables `QMK_TAG`, `QMK_SHA` (sourceable; also `grep -E '^[A-Z_]+='`-safe for `$GITHUB_ENV`). Consumed by `flash.sh` (Task 3) and CI (Task 4).

- [ ] **Step 1: Confirm the current checkout is at the intended rev**

Run:

```bash
git -C ~/qmk_firmware describe --tags && git -C ~/qmk_firmware rev-parse HEAD
```

Expected: `0.33.8` and `b1093e9da5f27253f3db692352daf5cf4ad5b98d`. If they differ, stop and reconcile before writing the pin.

- [ ] **Step 2: Create `scripts/qmk.pin`**

```sh
# QMK revision this firmware is built + verified against.
# Bump deliberately, then: ./scripts/flash.sh setup && ./scripts/flash.sh compile, re-verify on hardware.
QMK_TAG=0.33.8
QMK_SHA=b1093e9da5f27253f3db692352daf5cf4ad5b98d
# Toolchain provenance (CI pins these via the qmk_cli container image):
#   qmk CLI 1.1.8 · arm-none-eabi-gcc 9.2.1
```

- [ ] **Step 3: Verify it sources cleanly and matches the checkout**

Run:

```bash
( set -u; source scripts/qmk.pin; \
  echo "tag=$QMK_TAG sha=$QMK_SHA"; \
  test "$(git -C ~/qmk_firmware rev-parse HEAD)" = "$QMK_SHA" && echo "MATCH" )
```

Expected: prints `tag=0.33.8 sha=b1093e9...` then `MATCH`.

- [ ] **Step 4: Verify the `$GITHUB_ENV` filter drops comments**

Run:

```bash
grep -E '^[A-Z_]+=' scripts/qmk.pin
```

Expected: exactly two lines (`QMK_TAG=...`, `QMK_SHA=...`), no `#` lines.

- [ ] **Step 5: Commit**

```bash
git add scripts/qmk.pin
git commit -m "tooling: pin QMK revision (0.33.8) as build source of truth"
```

---

### Task 2: Format/whitespace config

**Files:**

- Create: `.editorconfig`
- Create: `.clang-format`

**Interfaces:**

- Produces: repo-root `.clang-format` used by `flash.sh format` (Task 3). No code consumes `.editorconfig` directly (editors/tools read it).

- [ ] **Step 1: Create `.editorconfig`**

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

- [ ] **Step 2: Copy QMK's own `.clang-format` verbatim**

Reusing the pinned tree's config guarantees `format` never diverges from the compiler's conventions.

Run:

```bash
test -f ~/qmk_firmware/.clang-format && cp ~/qmk_firmware/.clang-format .clang-format
```

Expected: file copied, no error. If the source is missing, stop — the checkout is not the pinned QMK tree.

- [ ] **Step 3: Verify clang-format accepts the config (parse check, no file changes)**

Run:

```bash
command -v clang-format >/dev/null && clang-format --dry-run firmware/gati/config.h >/dev/null && echo "CONFIG OK" \
  || echo "clang-format missing — install with: brew install clang-format"
```

Expected: `CONFIG OK` (or the install hint if clang-format is absent — acceptable; the config is still valid).

- [ ] **Step 4: Commit**

```bash
git add .editorconfig .clang-format
git commit -m "tooling: add .editorconfig + QMK-matched .clang-format"
```

---

### Task 3: Rewrite `flash.sh` as a multi-command tool

**Files:**

- Modify (full rewrite): `scripts/flash.sh`

**Interfaces:**

- Consumes: `scripts/qmk.pin` (`QMK_TAG`, `QMK_SHA`); repo-root `.clang-format` (Task 2).
- Produces: subcommands `compile`, `flash`, `setup`, `doctor`, `lint`, `format [--check]`, `size`, plus usage. `compile`/`flash` behavior unchanged from v1.1 (symlink + `:leave` watchdog) except for an added pin-drift warning. No-arg now prints usage (was: silently flashed).

- [ ] **Step 1: Capture the byte-for-byte baseline BEFORE editing**

Run with the _current_ script:

```bash
./scripts/flash.sh compile
shasum -a 256 ~/qmk_firmware/gmmk_pro_rev1_ansi_gati.bin | tee /private/tmp/claude-501/-Users-gati3478-Development-Keyboard-GMMK-Pro/c7a9ba06-7fa8-40bf-a284-06bc294d8297/scratchpad/bin.sha
```

Expected: a sha256 for the `44692`-byte image, saved to `scratchpad/bin.sha`.

- [ ] **Step 2: Rewrite `scripts/flash.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Pinned QMK revision — single source of truth, shared with CI.
# shellcheck source=qmk.pin
source "$REPO/scripts/qmk.pin"

QMK_HOME="${QMK_HOME:-$HOME/qmk_firmware}"
# Mainline QMK split gmmk/pro into rev1/rev2; `gmmk/pro/ansi` is an alias that
# resolves to `gmmk/pro/rev1/ansi` (our board). The alias works for -kb, but the
# keymap must physically live under the real rev1 path, so symlink there.
QMK_KM="$QMK_HOME/keyboards/gmmk/pro/rev1/ansi/keymaps/gati"
KB="gmmk/pro/ansi"
KM="gati"
BIN_NAME="gmmk_pro_rev1_ansi_gati.bin"
FLASH_BUDGET=262144   # STM32F303 (GMMK Pro): 256 KB flash

link_keymap() {
  mkdir -p "$(dirname "$QMK_KM")"
  ln -sfn "$REPO/firmware/gati" "$QMK_KM"   # repo stays the source of truth
}

# Warn (never block) when the local QMK checkout has drifted from the pinned rev.
# Drift can be intentional (testing a newer QMK), so this is advisory — but it makes
# silent divergence impossible. A missing checkout is fatal: nothing can build.
pin_check() {
  if [ ! -d "$QMK_HOME/.git" ]; then
    echo ">> flash.sh: no QMK checkout at $QMK_HOME. Run: $0 setup" >&2
    exit 1
  fi
  local head
  head="$(git -C "$QMK_HOME" rev-parse HEAD)"
  if [ "$head" != "$QMK_SHA" ]; then
    printf '>> flash.sh: WARNING — QMK checkout drifted from the pinned rev.\n   pinned : %s (%s)\n   actual : %s\n   Fix:     %s setup   (proceeding anyway)\n' \
      "$QMK_SHA" "$QMK_TAG" "$head" "$0" >&2
  fi
}

# dfu-util intermittently wedges on the post-write `:leave` USB reset (macOS). The write is
# already complete by the time dfu-util prints its success marker, so watch for the marker and
# kill a still-stuck dfu-util after a short grace. See CLAUDE.md Gotchas. (Unchanged from v1.1.)
flash_gmmk() {
  local grace="${FLASH_LEAVE_GRACE:-10}"
  local marker="File downloaded successfully"
  local logf killed_ok=0 job waited st
  logf="$(mktemp "${TMPDIR:-/tmp}/gmmk-flash.XXXXXX")"

  # process substitution (not `| tee`) so $! stays qmk's pid — real flash failures
  # must still surface instead of being masked by tee's always-zero exit.
  qmk flash -kb "$KB" -km "$KM" > >(tee "$logf") 2>&1 &
  job=$!

  while kill -0 "$job" 2>/dev/null; do
    if grep -q "$marker" "$logf" 2>/dev/null; then
      waited=0
      while kill -0 "$job" 2>/dev/null && [ "$waited" -lt "$grace" ]; do
        sleep 1; waited=$((waited + 1))
      done
      if kill -0 "$job" 2>/dev/null; then
        printf '\n>> flash.sh: firmware written & verified; dfu-util stuck on :leave — killing after %ss (expected on macOS; the board has already rebooted into the new firmware).\n' "$grace"
        pkill -f 'dfu-util.*gmmk_pro' 2>/dev/null || true
        killed_ok=1
      fi
      break
    fi
    sleep 1
  done

  set +e
  wait "$job"
  st=$?
  set -e
  rm -f "$logf"

  [ "$killed_ok" -eq 1 ] && return 0
  return "$st"
}

cmd_setup() {
  if [ ! -d "$QMK_HOME/.git" ]; then
    echo ">> flash.sh: cloning QMK $QMK_TAG into $QMK_HOME (this is large)"
    git clone --branch "$QMK_TAG" --recurse-submodules \
      https://github.com/qmk/qmk_firmware "$QMK_HOME"
  else
    if [ -n "$(git -C "$QMK_HOME" status --porcelain)" ]; then
      echo ">> flash.sh: $QMK_HOME has local changes — refusing to touch it. Resolve them, then rerun setup." >&2
      exit 1
    fi
    git -C "$QMK_HOME" fetch --tags origin
    git -C "$QMK_HOME" checkout "$QMK_SHA"
    git -C "$QMK_HOME" submodule update --init --recursive
  fi
  local head
  head="$(git -C "$QMK_HOME" rev-parse HEAD)"
  if [ "$head" = "$QMK_SHA" ]; then
    echo ">> flash.sh: QMK checkout at pinned rev $QMK_TAG ($QMK_SHA)."
  else
    echo ">> flash.sh: ERROR — after setup HEAD is $head, expected $QMK_SHA." >&2
    exit 1
  fi
}

cmd_size() {
  qmk compile -kb "$KB" -km "$KM" >/dev/null
  local bin size pct
  bin="$QMK_HOME/$BIN_NAME"
  [ -f "$bin" ] || { echo ">> flash.sh: $bin not found after compile." >&2; exit 1; }
  size="$(wc -c < "$bin" | tr -d ' ')"
  pct=$(( size * 100 / FLASH_BUDGET ))
  printf '>> flash.sh: %s = %s bytes (%s%% of %s KB flash)\n' \
    "$BIN_NAME" "$size" "$pct" "$(( FLASH_BUDGET / 1024 ))"
}

cmd_format() {
  command -v clang-format >/dev/null || {
    echo ">> flash.sh: clang-format not found. Install: brew install clang-format" >&2
    exit 1
  }
  local files=("$REPO/firmware/gati/keymap.c" "$REPO/firmware/gati/config.h")
  if [ "${1:-}" = "--check" ]; then
    clang-format --dry-run --Werror "${files[@]}"
  else
    clang-format -i "${files[@]}"
  fi
}

usage() {
  cat >&2 <<'EOF'
usage: flash.sh <command>

  compile            build the firmware (warns on QMK pin drift)
  flash              build + flash (put the board in bootloader first: Fn+B)
  setup              clone/checkout ~/qmk_firmware to the pinned QMK rev
  doctor             run qmk doctor (toolchain preflight)
  lint               run qmk lint on the keymap (advisory)
  format [--check]   clang-format firmware/gati C sources (--check = dry run, no writes)
  size               build + report firmware size vs the 256 KB flash budget
EOF
  exit "${1:-1}"
}

case "${1:-help}" in
  compile) pin_check; link_keymap; qmk compile -kb "$KB" -km "$KM" ;;
  flash)   pin_check; link_keymap; flash_gmmk ;;
  setup)   cmd_setup ;;
  doctor)  qmk doctor ;;
  lint)    link_keymap; qmk lint -kb "$KB" -km "$KM" ;;
  format)  cmd_format "${2:-}" ;;
  size)    pin_check; link_keymap; cmd_size ;;
  help|-h|--help) usage 0 ;;
  *)       echo ">> flash.sh: unknown command '${1}'" >&2; usage 1 ;;
esac
```

- [ ] **Step 3: Keep it executable**

Run:

```bash
chmod +x scripts/flash.sh
```

- [ ] **Step 4: Regression — `compile` is byte-identical**

Run:

```bash
./scripts/flash.sh compile
shasum -a 256 -c /private/tmp/claude-501/-Users-gati3478-Development-Keyboard-GMMK-Pro/c7a9ba06-7fa8-40bf-a284-06bc294d8297/scratchpad/bin.sha
```

Expected: ends with `: OK` (same SHA as the baseline from Step 1).

- [ ] **Step 5: `pin_check` warns on drift but still builds**

Simulate a drifted checkout with a throwaway git repo (does not touch `~/qmk_firmware`):

```bash
D=$(mktemp -d); git -C "$D" init -q; git -C "$D" -c user.email=t@t -c user.name=t commit -q --allow-empty -m x
QMK_HOME="$D" ./scripts/flash.sh compile 2>&1 | grep -q "drifted from the pinned rev" && echo "WARNS OK"
rm -rf "$D"
```

Expected: prints `WARNS OK` (the warning fires; the subsequent compile failing in the fake tree is irrelevant to this assertion).

- [ ] **Step 6: usage + unknown-command behavior**

Run:

```bash
./scripts/flash.sh help >/dev/null; echo "help exit=$?"          # expect 0
./scripts/flash.sh bogus 2>&1 | grep -q "unknown command" && echo "UNKNOWN OK"
```

Expected: `help exit=0` and `UNKNOWN OK`.

- [ ] **Step 7: `size`, `doctor`, `lint`, `format --check` smoke tests**

Run:

```bash
./scripts/flash.sh size   | grep -q "% of 256 KB flash" && echo "SIZE OK"
./scripts/flash.sh doctor 2>&1 | grep -qi "qmk"          && echo "DOCTOR OK"
./scripts/flash.sh lint   2>&1 | tail -3; echo "lint exit=$?"   # advisory; record output
./scripts/flash.sh format --check 2>&1 | tail -3; echo "format-check ran (nonzero if reflow needed — expected, not applied)"
```

Expected: `SIZE OK`, `DOCTOR OK`, lint output shown, and `format --check` runs (a nonzero exit here is fine and expected — the hand-aligned sources differ from clang-format canonical; we intentionally do **not** apply it).

- [ ] **Step 8: `setup` is a safe no-op on the already-pinned clean checkout**

Run (only if `~/qmk_firmware` is clean — `git -C ~/qmk_firmware status --porcelain` empty):

```bash
./scripts/flash.sh setup && git -C ~/qmk_firmware rev-parse HEAD
```

Expected: reports "at pinned rev" and HEAD is still `b1093e9...`. (If the checkout has local changes, `setup` will refuse — that is correct behavior; skip this step and note it.)

- [ ] **Step 9: Commit**

```bash
git add scripts/flash.sh
git commit -m "tooling: flash.sh subcommands (setup/doctor/lint/format/size) + pin-drift warning"
```

---

### Task 4: CI — compile on push and upload the firmware

**Files:**

- Create: `.github/workflows/build.yml`

**Interfaces:**

- Consumes: `scripts/qmk.pin` (via `grep`-into-`$GITHUB_ENV`).
- Produces: a `build` workflow uploading artifact `gmmk_pro_rev1_ansi_gati` and a run URL for the README badge (Task 5).

- [ ] **Step 1: Create `.github/workflows/build.yml`**

```yaml
name: build
on: [push, pull_request, workflow_dispatch]

jobs:
  build:
    runs-on: ubuntu-latest
    container: ghcr.io/qmk/qmk_cli:latest # digest-pinned in Step 5 after the first green run
    steps:
      - name: Checkout firmware repo
        uses: actions/checkout@v4
        with:
          path: fw

      - name: Load pinned QMK revision
        run: grep -E '^[A-Z_]+=' fw/scripts/qmk.pin >> "$GITHUB_ENV"

      - name: Clone pinned QMK (shallow, with submodules)
        run: |
          git clone --depth 1 --branch "$QMK_TAG" \
            --recurse-submodules --shallow-submodules \
            https://github.com/qmk/qmk_firmware qmk
          test "$(git -C qmk rev-parse HEAD)" = "$QMK_SHA" \
            || { echo "::error::QMK tag $QMK_TAG no longer resolves to $QMK_SHA"; exit 1; }

      - name: Symlink keymap into the QMK tree
        run: ln -sfn "$PWD/fw/firmware/gati" qmk/keyboards/gmmk/pro/rev1/ansi/keymaps/gati

      - name: Compile
        working-directory: qmk
        run: qmk compile -kb gmmk/pro/ansi -km gati

      - name: Lint (advisory)
        working-directory: qmk
        run: qmk lint -kb gmmk/pro/ansi -km gati || true

      - name: Report size
        run: |
          bin="qmk/gmmk_pro_rev1_ansi_gati.bin"
          size=$(wc -c < "$bin")
          echo "### Firmware: $size bytes ($((size * 100 / 262144))% of 256 KB)" >> "$GITHUB_STEP_SUMMARY"

      - name: Upload firmware artifact
        uses: actions/upload-artifact@v4
        with:
          name: gmmk_pro_rev1_ansi_gati
          path: qmk/gmmk_pro_rev1_ansi_gati.bin
          if-no-files-found: error
```

- [ ] **Step 2: Validate the YAML locally**

Run:

```bash
python3 -c "import yaml,sys; yaml.safe_load(open('.github/workflows/build.yml')); print('YAML OK')"
```

Expected: `YAML OK`.

- [ ] **Step 3: Commit and push the branch**

```bash
git add .github/workflows/build.yml
git commit -m "ci: compile firmware on push in pinned qmk_cli container, upload .bin"
git push -u origin tooling-overhaul
```

- [ ] **Step 4: Watch the run go green and confirm the artifact + size**

Run:

```bash
gh run watch --exit-status $(gh run list --branch tooling-overhaul --limit 1 --json databaseId -q '.[0].databaseId')
gh run download --name gmmk_pro_rev1_ansi_gati --dir /private/tmp/claude-501/-Users-gati3478-Development-Keyboard-GMMK-Pro/c7a9ba06-7fa8-40bf-a284-06bc294d8297/scratchpad/ci-bin
wc -c /private/tmp/claude-501/-Users-gati3478-Development-Keyboard-GMMK-Pro/c7a9ba06-7fa8-40bf-a284-06bc294d8297/scratchpad/ci-bin/gmmk_pro_rev1_ansi_gati.bin
```

Expected: the run exits green; the downloaded `.bin` is `44692` bytes (same as local). If `gh` is unavailable, verify via the Actions web UI instead.

- [ ] **Step 5: Pin the container digest**

From the green run's "Initialize containers" log, copy the resolved `ghcr.io/qmk/qmk_cli@sha256:<digest>` and replace the `container:` line, so the toolchain is fully reproducible.

```bash
# edit .github/workflows/build.yml: container: ghcr.io/qmk/qmk_cli@sha256:<digest>
git add .github/workflows/build.yml
git commit -m "ci: pin qmk_cli container by digest"
git push
```

Expected: re-push triggers another green run on the digest-pinned image.

---

### Task 5: Documentation

**Files:**

- Modify: `CLAUDE.md` (Build & flash section)
- Modify: `README.md` (add CI badge)
- Modify: `docs/PLAN.md` and `docs/DESIGN.md` (§12 toolchain) — reference the pin + CI

**Interfaces:**

- Consumes: the final `flash.sh` command list (Task 3), the workflow name `build` and repo `gati3478/gmmk-pro-qmk` (Task 4).

- [ ] **Step 1: Update `CLAUDE.md` Build & flash section**

Replace the two-command build block and add pin/setup/CI notes. New content for the "Build & flash" section:

````markdown
## Build & flash

```bash
./scripts/flash.sh compile     # build only (warns if the QMK checkout drifts from the pin)
./scripts/flash.sh flash       # build + flash (put board in bootloader first: Fn+B)
./scripts/flash.sh setup       # bring ~/qmk_firmware to the pinned QMK rev (new machine / after a bump)
./scripts/flash.sh size        # build + report firmware size vs the 256 KB budget
./scripts/flash.sh lint        # qmk lint (advisory)
./scripts/flash.sh format      # clang-format the C sources (opt-in; reflows hand-aligned blocks)
```

- **QMK is pinned** in `scripts/qmk.pin` (`0.33.8` / `b1093e9…`). It is the single source of truth
  shared by `flash.sh` and CI. On a fresh Mac: `brew install qmk/qmk/qmk` → `./scripts/flash.sh setup`.
- **CI** (`.github/workflows/build.yml`) compiles on every push in QMK's pinned container and uploads
  `gmmk_pro_rev1_ansi_gati.bin` as an artifact; the SHA-verify step fails loudly if the tag ever moves.
- `flash.sh` symlinks `firmware/gati/` into `~/qmk_firmware/.../keymaps/gati`, so this repo stays the
  source of truth and QMK is just the compiler.
````

- [ ] **Step 2: Add a "Bumping QMK" line to CLAUDE.md Gotchas**

Add under Gotchas:

```markdown
- **Bumping QMK:** edit `scripts/qmk.pin` (tag + SHA), `./scripts/flash.sh setup`, then compile,
  re-verify on hardware, reflash. CI's SHA-verify guards against a tag silently moving underneath you.
```

- [ ] **Step 3: Add the CI badge to `README.md`**

Add near the top (adjust the default branch if not `main`):

```markdown
[![build](https://github.com/gati3478/gmmk-pro-qmk/actions/workflows/build.yml/badge.svg)](https://github.com/gati3478/gmmk-pro-qmk/actions/workflows/build.yml)
```

- [ ] **Step 4: Reference the pin + CI in `docs/PLAN.md` and `docs/DESIGN.md`**

In `docs/PLAN.md` Architecture paragraph and `docs/DESIGN.md` §12 (toolchain), add one line each noting the QMK revision is pinned in `scripts/qmk.pin` and built in CI. Keep wording consistent with CLAUDE.md (SSOT discipline).

- [ ] **Step 5: Consistency check — docs match the tool**

Run:

```bash
./scripts/flash.sh help 2>&1 | grep -oE '^\s+(compile|flash|setup|doctor|lint|format|size)' | sort -u
grep -oE 'flash\.sh (compile|flash|setup|doctor|lint|format|size)' CLAUDE.md | sort -u
```

Expected: every subcommand documented in CLAUDE.md exists in `flash.sh help` and vice-versa.

- [ ] **Step 6: Commit**

```bash
git add CLAUDE.md README.md docs/PLAN.md docs/DESIGN.md
git commit -m "docs: pin + CI + flash.sh subcommands in CLAUDE.md, README badge, DESIGN/PLAN refs"
```

---

## Final verification (whole feature)

- [ ] `./scripts/flash.sh compile` → byte-identical `44692`-byte image (SHA matches Task 3 baseline).
- [ ] `./scripts/flash.sh help` lists all seven subcommands; unknown commands print usage and exit nonzero.
- [ ] Pin drift warns but does not block; `setup` reconciles a clean checkout to the pin.
- [ ] GitHub Actions `build` is green on `tooling-overhaul`, artifact downloads and is `44692` bytes.
- [ ] Container pinned by digest; docs (CLAUDE.md / README / DESIGN / PLAN) consistent with the tool.
- [ ] Optional but recommended: flash the byte-identical firmware once to confirm hardware is unaffected.
- [ ] Merge `tooling-overhaul` → `main` via `superpowers:finishing-a-development-branch`.

```

```
