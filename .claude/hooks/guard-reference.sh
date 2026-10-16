#!/usr/bin/env bash
# PreToolUse guard (Edit|Write): this repo's reference/ is frozen provenance (docs/DESIGN.md §3).
# Exit 2 blocks the tool call; anything that prevents a verdict also blocks (fail closed).
set -u

refuse() {
  echo "reference/ guard: $1" >&2
  exit 2
}

command -v jq >/dev/null 2>&1 || refuse "jq not found, refusing Edit/Write until it is installed."

root="${CLAUDE_PROJECT_DIR:-$PWD}"
root_physical="$(cd "$root" 2>/dev/null && pwd -P)" || refuse "project dir unreadable, refusing."

# Normalize lexically (., .., //), resolve relative paths against the project, and compare
# case-insensitively (APFS), so firmware/../reference/x and REFERENCE/x are blocked too.
verdict="$(jq -er --arg root "$root" --arg physical "$root_physical" '
  def norm:
    split("/")
    | reduce (.[] | select(. != "" and . != ".")) as $s ([]; if $s == ".." then .[:-1] else . + [$s] end)
    | "/" + join("/") | ascii_downcase;
  (.tool_input.file_path // "") as $p
  | if $p == "" then "allow"
    else
      ((if ($p | startswith("/")) then $p else $root + "/" + $p end) | norm) as $f
      | if [$root, $physical] | map(norm + "/reference") | any(. as $r | $f == $r or ($f | startswith($r + "/")))
        then "block" else "allow" end
    end
')" || refuse "unreadable hook input, refusing."

[ "$verdict" = allow ] || refuse "reference/ is frozen provenance (recovered original firmware): never edit it. See docs/DESIGN.md §3."
