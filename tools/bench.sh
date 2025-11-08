#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="out"
mkdir -p "$OUT_DIR"

ptime() { /usr/bin/time -f "%e" "$@" 1>/dev/null 2>&1; }

bench_wfg() {
  local of="$OUT_DIR/bench_wfg.tsv"
  : > "$of"
  echo -e "case\tN\tE\tsec" >> "$of"

  for N in 200 500 1000; do
    for E in $((N*2)) $((N*4)); do
      ./bin/gen_wfg "$N" "$E" --seed 1 --mode random > "$OUT_DIR/w_${N}_${E}.in"
      ./bin/gen_wfg "$N" "$E" --seed 1 --mode cycle  --cycle-len 4 > "$OUT_DIR/wc_${N}_${E}.in"

      local t1 t2
      t1=$(ptime ./bin/detect_wfg "$OUT_DIR/w_${N}_${E}.in" || true)
      t2=$(ptime ./bin/detect_wfg "$OUT_DIR/wc_${N}_${E}.in" || true)
      echo -e "random\t$N\t$E\t$t1" >> "$of"
      echo -e "cycle4\t$N\t$E\t$t2" >> "$of"
    done
  done
  echo "Wrote $of"
}

bench_matrix() {
  local of="$OUT_DIR/bench_matrix.tsv"
  : > "$of"
  echo -e "case\tN\tM\tsec" >> "$of"

  for N in 50 100 200; do
    for M in 5 10; do
      ./bin/gen_matrix "$N" "$M" --seed 7 --mode ok   > "$OUT_DIR/m_ok_${N}_${M}.in"
      ./bin/gen_matrix "$N" "$M" --seed 7 --mode dead > "$OUT_DIR/m_dead_${N}_${M}.in"
      local t1 t2
      t1=$(ptime ./bin/detect_matrix "$OUT_DIR/m_ok_${N}_${M}.in" || true)
      t2=$(ptime ./bin/detect_matrix "$OUT_DIR/m_dead_${N}_${M}.in" || true)
      echo -e "ok\t$N\t$M\t$t1" >> "$of"
      echo -e "dead\t$N\t$M\t$t2" >> "$of"
    done
  done
  echo "Wrote $of"
}

bench_runtime() {
  local of="$OUT_DIR/bench_runtime.tsv"
  : > "$of"
  echo -e "case\tsec" >> "$of"

  local t0 t1 t2
  t0=$(ptime ./bin/demo_nocycle || true)
  t1=$(ptime env DD_LOG_LEVEL=0 LD_PRELOAD="$PWD/bin/libdd.so" ./bin/demo_nocycle || true)

  # deadlock case – không để treo quá lâu
  t2=$( (timeout 1 env DD_LOG_LEVEL=0 LD_PRELOAD="$PWD/bin/libdd.so" ./bin/demo_deadlock; echo $? ) 2>/dev/null | awk '{print "1.00"}' )

  echo -e "nocycle(no-preload)\t$t0" >> "$of"
  echo -e "nocycle(preload)\t$t1" >> "$of"
  echo -e "deadlock(preload)\t$t2" >> "$of"
  echo "Wrote $of"
}

case "${1:-all}" in
  wfg) bench_wfg ;;
  mx|matrix) bench_matrix ;;
  rt|runtime) bench_runtime ;;
  all) bench_wfg; bench_matrix; bench_runtime ;;
  *) echo "Usage: $0 [wfg|matrix|runtime|all]"; exit 1 ;;
esac
