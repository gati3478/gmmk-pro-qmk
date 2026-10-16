#!/usr/bin/env bash
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Pinned QMK revision — single source of truth, shared with CI.
# shellcheck source-path=SCRIPTDIR
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
  if [ ! -e "$QMK_HOME/.git" ]; then   # -e: a git worktree's .git is a file
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

# qmk picks its tree from the cwd before QMK_HOME, so run it from the checkout pin_check inspected.
prepare() {
  pin_check
  link_keymap
  cd "$QMK_HOME"
}

# install.qmk.fm always fetches the latest toolchain; GCC stamps its version into the .elf's
# .comment section, so check the compiler that actually built the image against the pin.
toolchain_check() {
  local used
  used="$(grep -a -o 'GCC: ([^)]*) [0-9.]*' "$QMK_HOME/.build/${BIN_NAME%.bin}.elf" | head -1 | sed 's/.* //')" || true
  if [ "$used" != "$QMK_GCC" ]; then
    printf '>> flash.sh: WARNING — built with arm-none-eabi-gcc %s, pinned %s (scripts/qmk.pin).\n' \
      "${used:-unknown}" "$QMK_GCC" >&2
  fi
}

build() {
  qmk compile -kb "$KB" -km "$KM"
  toolchain_check
}

# Every descendant of pid $1 (qmk → make → bootloader-wait loop → dfu-util).
descendants() {
  local c
  for c in $(pgrep -P "$1"); do
    echo "$c"
    descendants "$c"
  done
}

# dfu-util intermittently wedges on the post-write `:leave` USB reset (macOS). The write is
# already complete by the time dfu-util prints its success marker, so watch for the marker and
# kill a still-stuck dfu-util after a short grace. See CLAUDE.md Gotchas.
flash_gmmk() {
  local grace="${FLASH_LEAVE_GRACE:-10}"
  local marker="File downloaded successfully"
  local logf killed_ok=0 job waited st
  logf="$(mktemp "${TMPDIR:-/tmp}/gmmk-flash.XXXXXX")"

  # process substitution (not `| tee`) so $! stays qmk's pid — real flash failures
  # must still surface instead of being masked by tee's always-zero exit.
  qmk flash -kb "$KB" -km "$KM" > >(tee "$logf") 2>&1 &
  job=$!

  # A script's background jobs ignore SIGINT, so Ctrl-C would orphan qmk/dfu-util and a later
  # Fn+B would get flashed unsupervised. Tear the whole tree down instead.
  # shellcheck disable=SC2046  # word-splitting the pid list is intended
  # `|| true`: under set -e a pid that already exited would abort the handler before cleanup.
  trap 'kill $(descendants "$job") "$job" 2>/dev/null || true; rm -f "$logf"; exit 130' INT TERM

  while kill -0 "$job" 2>/dev/null; do
    if grep -q "$marker" "$logf" 2>/dev/null; then
      waited=0
      while kill -0 "$job" 2>/dev/null && [ "$waited" -lt "$grace" ]; do
        sleep 1; waited=$((waited + 1))
      done
      if kill -0 "$job" 2>/dev/null; then
        printf '\n>> flash.sh: firmware written; dfu-util stuck on :leave — killing after %ss (expected on macOS; the board has already rebooted into the new firmware).\n' "$grace"
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
  trap - INT TERM
  rm -f "$logf"

  [ "$killed_ok" -eq 1 ] && return 0
  return "$st"
}

cmd_setup() {
  if [ ! -e "$QMK_HOME/.git" ]; then
    echo ">> flash.sh: cloning QMK $QMK_TAG into $QMK_HOME (this is large)"
    git clone --branch "$QMK_TAG" --recurse-submodules \
      https://github.com/qmk/qmk_firmware "$QMK_HOME"
  else
    # Only tracked-file modifications count as "dirty": flash.sh itself plants an untracked
    # keymap symlink under keyboards/, which must never block a re-sync to the pinned rev.
    if ! git -C "$QMK_HOME" diff --quiet || ! git -C "$QMK_HOME" diff --cached --quiet; then
      echo ">> flash.sh: $QMK_HOME has uncommitted changes to tracked files — refusing to touch it. Resolve them, then rerun setup." >&2
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
  build >/dev/null
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
  case "${1:-}" in
    --check) clang-format --dry-run --Werror "${files[@]}" ;;
    "")      clang-format -i "${files[@]}" ;;
    *)       echo ">> flash.sh: unknown format option '$1' (only --check)" >&2; exit 1 ;;
  esac
}

usage() {
  local code="${1:-1}" fd=2
  [ "$code" -eq 0 ] && fd=1   # explicit `help` → stdout; error-triggered usage → stderr
  cat >&"$fd" <<'EOF'
usage: flash.sh <command>

  compile            build the firmware (warns on QMK pin / toolchain drift)
  flash              build + flash (bootloader first: hold Fn+B 1 s, or Esc while plugging in)
  setup              clone/checkout ~/qmk_firmware to the pinned QMK rev
  doctor             run qmk doctor (toolchain preflight)
  lint               run qmk lint on the keymap (advisory)
  format [--check]   clang-format firmware/gati C sources (--check = dry run, no writes)
  size               build + report firmware size vs the 256 KB flash budget
EOF
  exit "$code"
}

case "${1:-help}" in
  compile) prepare; build ;;
  flash)   prepare; build; flash_gmmk ;;   # build first so toolchain drift warns before writing
  setup)   cmd_setup ;;
  doctor)  qmk doctor ;;
  lint)    prepare; qmk lint -kb "$KB" -km "$KM" ;;
  format)  cmd_format "${2:-}" ;;
  size)    prepare; cmd_size ;;
  help|-h|--help) usage 0 ;;
  *)       echo ">> flash.sh: unknown command '${1}'" >&2; usage 1 ;;
esac
