#!/usr/bin/env bash
set -euo pipefail
BIN=./bin
ok=0; total=0

run_case() {
  local exe="$1" in="$2" out="$3"
  total=$((total+1))
  local got
  if ! got="$("$exe" "$in" 2>/dev/null)"; then
    echo "[FAIL] $(basename "$exe") $(basename "$in")"
    return
  fi
  local want; want="$(cat "$out")"

  # Chuẩn hoá so sánh mềm cho DEADLOCK (matrix có case chỉ ghi DEADLOCK)
  if [[ "$want" == "DEADLOCK" ]]; then
    if [[ "$got" =~ ^DEADLOCK($|[[:space:]]) ]]; then
      echo "[OK]   $(basename "$exe") $(basename "$in")"; ok=$((ok+1)); return
    fi
  fi

  if [[ "$got" == "$want" ]]; then
    echo "[OK]   $(basename "$exe") $(basename "$in")"; ok=$((ok+1))
  else
    echo "[FAIL] $(basename "$exe") $(basename "$in")"
  fi
}

# WFG
for t in tests/wfg/*.in; do
  run_case "$BIN/detect_wfg" "$t" "${t%.in}.out"
done
echo '---'
# Matrix
for t in tests/matrix/*.in; do
  run_case "$BIN/detect_matrix" "$t" "${t%.in}.out"
done

echo "$ok/$total tests passed"
