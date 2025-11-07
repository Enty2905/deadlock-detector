#!/usr/bin/env bash
# set -euo pipefail
pass=0; total=0
run_case () {
  local bin="$1" in="$2" out="$3"
  ((total++))
  if [[ ! -x "$bin" ]]; then echo "[SKIP] $(basename "$bin")"; return; fi
  got=$("$bin" "$in"); exp=$(cat "$out")
  if [[ "$got" == "$exp" ]]; then echo "[OK] $(basename "$bin") $(basename "$in")"; ((pass++))
  else echo "[FAIL] $(basename "$bin")"; echo " got: $got"; echo " exp: $exp"; fi
}
 for f in tests/wfg/*.in; do run_case ./bin/detect_wfg  "$f" "${f%.in}.out"; done
# [NEW] Thêm phần này vào để test detect_matrix
echo "---" 
for f in tests/matrix/*.in; do run_case ./bin/detect_matrix "$f" "${f%.in}.out"; done
# [NEW] Hết phần thêm

echo "$pass/$total tests passed"
[[ "$pass" -eq "$total" ]] || exit 1
