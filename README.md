
# Deadlock Detector — VKU 23IT248

Hệ thống phát hiện deadlock đa mô-đun:
- **Matrix mode** (Banker’s Detection variant)
- **WFG mode** (Wait-For Graph cycle detection)
- **Runtime mode (libdd.so)** dùng `LD_PRELOAD` để phát hiện deadlock thật giữa các thread.

---

##  I/O Format

### Matrix mode
**Input:**

N M
<Allocation[N][M]>
<Request/Need[N][M]>
<Available[M]>

**Output:**

NO DEADLOCK
hoặc
DEADLOCK on k process(es): P<i> P<j> ...


---

### Wait-For Graph (WFG) mode
**Input:**

P E
<u v> # nghĩa là Pu đang chờ Pv (0-index)

**Output:**

NO DEADLOCK
hoặc
DEADLOCK cycle: P<i0> P<i1> ... P<ik> P<i0>


---

##  Build

```bash
make clean && make

Các binary chính được sinh trong bin/:

    detect_matrix, detect_wfg

    ddetect (CLI hợp nhất)

    demo_deadlock, demo_deadlock3, demo_nocycle

    libdd.so (runtime detector)

    gen_wfg, gen_matrix (trình sinh dữ liệu ngẫu nhiên)

    wfg_to_dot, wfg_check (visualization)

    tools/dot2png.sh (chuyển DOT → PNG)

 Usage (Unified CLI)

./bin/ddetect --mode wfg    --file tests/wfg/01_cycle.in
./bin/ddetect --mode matrix --file tests/matrix/01_ok_simple.in

 Complexity
Module	Thuật toán	Độ phức tạp
WFG	DFS/back-edge	O(P + E)
Matrix	Banker’s detection	~O(N²·M)
Runtime (libdd)	Incremental cycle check	~O(V + E) mỗi lần lock
 Test tự động

./scripts/run_tests.sh

Chạy toàn bộ test trong thư mục tests/ và báo [OK]/[FAIL].
 Runtime Detector (LD_PRELOAD)

# Tình huống deadlock thật
timeout 2 ./bin/demo_deadlock || echo "(deadlock giả lập)"

# Dò vòng bằng libdd.so
DD_LOG_LEVEL=1 LD_PRELOAD=./bin/libdd.so ./bin/demo_deadlock
DD_LOG_LEVEL=1 LD_PRELOAD=./bin/libdd.so ./bin/demo_deadlock3
DD_LOG_LEVEL=1 LD_PRELOAD=./bin/libdd.so ./bin/demo_nocycle

DD_LOG_LEVEL:

    0 → tắt log

    1 → log cơ bản

    2 → log chi tiết từng mutex/thread

 Visualization (Graphviz)

Sinh .dot và .png minh họa chu trình:

./bin/wfg_check tests/wfg/01_cycle.in
./bin/wfg_to_dot tests/wfg/01_cycle.in out/wfg01.dot
./tools/dot2png.sh out/wfg01.dot

 Kết quả: out/wfg01.png thể hiện các node đỏ thuộc chu trình deadlock.
 Benchmark (T12)

Sinh dữ liệu lớn & đo hiệu năng:

bash tools/bench.sh all
head -n 5 out/bench_wfg.tsv
head -n 5 out/bench_matrix.tsv
cat out/bench_runtime.tsv

Output gồm:

    bench_wfg.tsv — thời gian phát hiện trên đồ thị lớn.

    bench_matrix.tsv — thời gian kiểm tra ma trận.

    bench_runtime.tsv — runtime preload/no-preload.

 Demo toàn bộ quy trình

#  Build sạch
make clean && make

#  Sinh dữ liệu WFG & Matrix
./bin/gen_wfg 6 8 --seed 1 --mode cycle --cycle-len 4 > out/wfg_sample.in
./bin/gen_matrix 5 3 --seed 7 --mode ok   > out/matrix_ok.in
./bin/gen_matrix 5 3 --seed 7 --mode dead > out/matrix_dead.in

#  Kiểm tra deadlock offline
./bin/detect_wfg out/wfg_sample.in
./bin/detect_matrix out/matrix_dead.in

#  Visualize WFG
./bin/wfg_to_dot out/wfg_sample.in out/wfg_sample.dot
./tools/dot2png.sh out/wfg_sample.dot

#  Runtime detection (libdd.so)
timeout 2 ./bin/demo_deadlock || echo "(treo do deadlock giả lập)"
DD_LOG_LEVEL=1 LD_PRELOAD="$PWD/bin/libdd.so" ./bin/demo_deadlock

#  Benchmark tổng hợp
bash tools/bench.sh all




