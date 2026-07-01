#!/usr/bin/env bash
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Mainline QMK split gmmk/pro into rev1/rev2; `gmmk/pro/ansi` is an alias that
# resolves to `gmmk/pro/rev1/ansi` (our board). The alias works for -kb, but the
# keymap must physically live under the real rev1 path, so symlink there.
QMK_KM="$HOME/qmk_firmware/keyboards/gmmk/pro/rev1/ansi/keymaps/gati"

mkdir -p "$(dirname "$QMK_KM")"
ln -sfn "$REPO/firmware/gati" "$QMK_KM"   # repo stays the source of truth

case "${1:-flash}" in
  compile) qmk compile -kb gmmk/pro/ansi -km gati ;;
  flash)   qmk flash   -kb gmmk/pro/ansi -km gati ;;
  *) echo "usage: flash.sh [compile|flash]"; exit 1 ;;
esac
