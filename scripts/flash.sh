#!/usr/bin/env bash
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Mainline QMK split gmmk/pro into rev1/rev2; `gmmk/pro/ansi` is an alias that
# resolves to `gmmk/pro/rev1/ansi` (our board). The alias works for -kb, but the
# keymap must physically live under the real rev1 path, so symlink there.
QMK_KM="$HOME/qmk_firmware/keyboards/gmmk/pro/rev1/ansi/keymaps/gati"

mkdir -p "$(dirname "$QMK_KM")"
ln -sfn "$REPO/firmware/gati" "$QMK_KM"   # repo stays the source of truth

# On macOS, dfu-util reliably wedges on the post-write `:leave` USB reset: the board
# detaches + reboots into the new firmware, but dfu-util never gets its detach ack and
# blocks forever (this once cost us a 57-minute zombie flash). The write is already
# complete by then. So stream the flash and, once dfu-util prints its success line, give
# it a short grace period to exit on its own — if it's still stuck, kill it. Killing
# after that marker is safe: the image is written+verified and the board has rebooted.
# A blanket `timeout` can't be used — `qmk flash` legitimately blocks, possibly for
# minutes, waiting for the board to enter bootloader; only the marker tells the phases
# apart. Grace is tunable via FLASH_LEAVE_GRACE (seconds).
flash_gmmk() {
  local grace="${FLASH_LEAVE_GRACE:-10}"
  local marker="File downloaded successfully"
  local logf killed_ok=0 job waited st
  logf="$(mktemp "${TMPDIR:-/tmp}/gmmk-flash.XXXXXX")"

  # process substitution (not `| tee`) so $! stays qmk's pid — real flash failures
  # must still surface instead of being masked by tee's always-zero exit.
  qmk flash -kb gmmk/pro/ansi -km gati > >(tee "$logf") 2>&1 &
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

  if [ "$killed_ok" -eq 1 ]; then
    return 0
  fi
  return "$st"
}

case "${1:-flash}" in
  compile) qmk compile -kb gmmk/pro/ansi -km gati ;;
  flash)   flash_gmmk ;;
  *) echo "usage: flash.sh [compile|flash]"; exit 1 ;;
esac
