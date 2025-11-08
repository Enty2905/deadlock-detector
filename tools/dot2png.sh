#!/usr/bin/env bash
set -euo pipefail
if ! command -v dot >/dev/null 2>&1; then
  echo "Graphviz 'dot' chưa cài. Cài: sudo apt-get install graphviz" >&2
  exit 1
fi
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
  echo "Usage: $0 input.dot [output.png]" >&2
  exit 1
fi
in="$1"
out="${2:-${in%.dot}.png}"
dot -Tpng "$in" -o "$out"
echo "Wrote $out"
