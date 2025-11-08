#!/usr/bin/env bash
set -euo pipefail

pass=0; total=0

run_case() {
  tool="$1"; in="$2"
  exp_file="${in%.in}.out"
  exp="$(tr -d '\r' < "$exp_file" | tr '[:lower:]' '[:upper:]')"
  out="$(./bin/$tool "$in" 2>/dev/null || true)"
  outU="$(printf "%s" "$out" | tr '[:lower:]' '[:upper:]')"

  want_deadlock=0
  echo "$exp" | grep -q "DEADLOCK" && want_deadlock=1

  if [ "$want_deadlock" -eq 1 ]; then
    if echo "$outU" | grep -q "DEADLOCK"; then
      echo "[OK] $tool $(basename "$in")"
      pass=$((pass+1))
    else
      echo "[FAIL] $tool $(basename "$in") -> expected DEADLOCK"
      echo "OUT: $out"
    fi
  else
    if echo "$outU" | grep -q "NO DEADLOCK"; then
      echo "[OK] $tool $(basename "$in")"
      pass=$((pass+1))
    else
      echo "[FAIL] $tool $(basename "$in") -> expected NO DEADLOCK"
      echo "OUT: $out"
    fi
  fi
  total=$((total+1))
}

for f in tests/wfg/*.in; do run_case detect_wfg "$f"; done
echo '---'
for f in tests/matrix/*.in; do run_case detect_matrix "$f"; done

echo "$pass/$total tests passed"
[ "$pass" -eq "$total" ]
