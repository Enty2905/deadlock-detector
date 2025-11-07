# deadlock-detector

## I/O Format

### Matrix mode
Input:
- Dòng 1: `N M`
- `N` dòng Allocation (M số mỗi dòng)
- `N` dòng Request/Need (M số mỗi dòng)
- Dòng cuối: `M` số Available

Output:
- `NO DEADLOCK` **hoặc**
- `DEADLOCK on k process(es): P<i> P<j> ...`

### Wait-For Graph (WFG) mode
Input:
- Dòng 1: `P E`
- `E` dòng `u v` nghĩa là `Pu` đang chờ `Pv` (0-index)

Output:
- `NO DEADLOCK` **hoặc**
- `DEADLOCK cycle: P<i0> P<i1> ... P<ik> P<i0>`

## Build
make

## Usage (Unified CLI)
Build:
  make
Run:
  ./bin/ddetect --mode wfg    --file tests/wfg/01_cycle.in
  ./bin/ddetect --mode matrix --file tests/matrix/01_ok_simple.in

## Complexity
- WFG cycle detection (DFS/back-edge): O(P + E)
- Matrix safety-check (Banker’s detection variant): ~ O(N^2 · M)

## Test
  ./scripts/run_tests.sh

## Runtime detector (LD_PRELOAD)
Build:
  make
Run demo:
  timeout 2 ./bin/demo_deadlock              # treo (deadlock thật)
  DD_LOG_LEVEL=1 LD_PRELOAD=./bin/libdd.so ./bin/demo_deadlock  # in cycle

Env:
  DD_LOG_LEVEL=0|1  (0=tắt log, 1=bật log)

### Day 7 — Runtime WFG (Thread↔Mutex)
- Trước `lock(m)`: thêm **T→M** (thread chờ mutex).
- Sau khi lock: gỡ **T→M**, thêm **M→T** (mutex thuộc thread).
- `unlock(m)`: gỡ **M→T`.
- Phát hiện chu trình ngay sau khi thêm **T→M**.
Demo:
  timeout 2 ./bin/demo_deadlock
  timeout 2 env DD_LOG_LEVEL=1 LD_PRELOAD=./bin/libdd.so ./bin/demo_deadlock
  timeout 2 ./bin/demo_deadlock3
  timeout 2 env DD_LOG_LEVEL=1 LD_PRELOAD=./bin/libdd.so ./bin/demo_deadlock3

### Day 8 — Chống false positive & Log chi tiết
- `trylock`: không chờ ⇒ **không** thêm cạnh T→M; nếu thành công, thêm **M→T**.
- `lock`: chỉ thêm cạnh **T→M** khi `owner != 0` **và** `owner != me` (đang bận bởi thread khác).
- Khi lock thành công: gỡ **T→M**, cập nhật owner, thêm **M→T**.
- Log chu trình dạng chuỗi node (TID & địa chỉ mutex đều hiển thị qua nhãn T…/M…).
- Demo 3 case:
  1) `demo_nocycle` (không vòng) → không có log chu trình.
  2) `demo_deadlock` (vòng 2 đỉnh) → có log chu trình.
  3) `demo_deadlock3` (vòng 3 đỉnh) → có log chu trình.
