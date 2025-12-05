# 📊 SCRIPT THUYẾT TRÌNH: PHÁT HIỆN DEADLOCK BẰNG THUẬT TOÁN MA TRẬN (BANKER'S ALGORITHM)

---

## 🎯 SLIDE 1: GIỚI THIỆU TỔNG QUAN

**Nói:**
> "Xin chào mọi người! Phần tiếp theo em sẽ trình bày về thuật toán phát hiện Deadlock sử dụng **Ma trận** - còn gọi là **Banker's Algorithm** (Thuật toán Ngân hàng). Đây là thuật toán kinh điển do Edsger Dijkstra đề xuất."

### Banker's Algorithm là gì?
- Thuật toán để **phát hiện** và **tránh deadlock** trong hệ thống có **nhiều instance** của tài nguyên
- Mô phỏng cách ngân hàng cho vay tiền: chỉ cho vay nếu đảm bảo có thể thu hồi

### Ý tưởng chính:
```
1. Giả lập việc cấp phát tài nguyên
2. Tìm tiến trình có thể hoàn thành với tài nguyên hiện có
3. Khi tiến trình hoàn thành → thu hồi tài nguyên
4. Lặp lại cho đến khi:
   - Tất cả hoàn thành → NO DEADLOCK ✅
   - Không ai có thể tiến triển → DEADLOCK ❌
```

---

## 🎯 SLIDE 2: CÁC MA TRẬN VÀ VECTOR CẦN THIẾT

**Nói:**
> "Trước tiên, hãy hiểu các cấu trúc dữ liệu được sử dụng trong thuật toán."

### Định nghĩa:
- **N** = Số tiến trình (P0, P1, ..., P(N-1))
- **M** = Số loại tài nguyên (R0, R1, ..., R(M-1))

### Các ma trận & vector:

| Tên | Kích thước | Ý nghĩa |
|-----|------------|---------|
| **Allocation (A)** | N × M | `A[i][j]` = Số tài nguyên loại j mà Pi **đang giữ** |
| **Need** | N × M | `Need[i][j]` = Số tài nguyên loại j mà Pi **còn cần** để hoàn thành |
| **Available (Avail)** | 1 × M | `Avail[j]` = Số tài nguyên loại j **đang rảnh** trong hệ thống |

### Ví dụ minh họa:
```
N = 3 tiến trình, M = 2 loại tài nguyên

Allocation:          Need:              Available:
      R0  R1              R0  R1              R0  R1
P0 [  1   0  ]      P0 [  1   1  ]          [  1   1  ]
P1 [  0   1  ]      P1 [  0   1  ]
P2 [  1   1  ]      P2 [  0   0  ]

Ý nghĩa:
- P0 đang giữ 1 R0, cần thêm 1 R0 và 1 R1
- P2 đang giữ 1 R0, 1 R1, không cần thêm gì → có thể hoàn thành!
```

---

## 🎯 SLIDE 3: CẤU TRÚC FILE INPUT

**Nói:**
> "File input có định dạng cụ thể để mô tả trạng thái hệ thống."

### Định dạng file:
```
N M                         ← Dòng 1: N tiến trình, M loại tài nguyên
A[0][0] A[0][1] ... A[0][M-1]    ← N dòng tiếp: Ma trận Allocation
A[1][0] A[1][1] ... A[1][M-1]
...
A[N-1][0] ...
Need[0][0] Need[0][1] ...        ← N dòng tiếp: Ma trận Need
Need[1][0] ...
...
Avail[0] Avail[1] ... Avail[M-1] ← Dòng cuối: Vector Available
```

### Ví dụ file input (01_ok_simple.in):
```
2 2           ← 2 tiến trình, 2 loại tài nguyên
0 1           ← P0 đang giữ: 0 R0, 1 R1
1 0           ← P1 đang giữ: 1 R0, 0 R1
0 0           ← P0 cần thêm: 0 R0, 0 R1 (không cần gì)
0 0           ← P1 cần thêm: 0 R0, 0 R1 (không cần gì)
1 1           ← Available: 1 R0, 1 R1

→ Cả P0 và P1 đều có thể hoàn thành → NO DEADLOCK
```

---

## 🎯 SLIDE 4: CODE - PHẦN 1: ĐỌC INPUT

**Nói:**
> "Bắt đầu phân tích code. Đầu tiên là phần đọc dữ liệu từ file."

### Code: Header và đọc file
```c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "util.h"      // Hàm cấp phát an toàn: xmalloc, xcalloc

int main(int argc, char** argv) {
    // Kiểm tra tham số dòng lệnh
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <input>\n", argv[0]);
        return 2;
    }
    
    // Mở file
    FILE* f = fopen(argv[1], "r");
    if(!f) { perror("fopen"); return 2; }
    
    // Đọc N (số tiến trình) và M (số loại tài nguyên)
    int N, M;
    if(fscanf(f, "%d %d", &N, &M) != 2) {
        fprintf(stderr, "Bad header\n");
        fclose(f); 
        return 2;
    }
```

### Các hàm util.h:
```c
// xcalloc: cấp phát và khởi tạo 0, thoát nếu lỗi
void* xcalloc(size_t n, size_t sz);

// Ví dụ: xcalloc(5, sizeof(int)) 
//        → cấp phát mảng 5 int, tất cả = 0
```

---

## 🎯 SLIDE 5: CODE - PHẦN 2: CẤP PHÁT MA TRẬN

**Nói:**
> "Tiếp theo là cấp phát bộ nhớ cho các ma trận và vector."

### Code:
```c
    // Cấp phát ma trận Allocation: N hàng
    int** A = (int**)xcalloc(N, sizeof(int*));
    
    // Cấp phát ma trận Need: N hàng  
    int** Need = (int**)xcalloc(N, sizeof(int*));
    
    // Cấp phát vector Available: M phần tử
    int* Avail = (int*)xcalloc(M, sizeof(int));
```

### Minh họa cấu trúc bộ nhớ:
```
A (con trỏ tới con trỏ):
┌───┐     ┌───┬───┬───┐
│ * │────→│ 1 │ 0 │ 2 │  A[0] - M phần tử
├───┤     └───┴───┴───┘
│ * │────→│ 0 │ 1 │ 1 │  A[1]
├───┤     └───┴───┴───┘
│ * │────→│ 2 │ 1 │ 0 │  A[2]
└───┘     └───┴───┴───┘
  N hàng      M cột
```

---

## 🎯 SLIDE 6: CODE - PHẦN 3: ĐỌC DỮ LIỆU VÀO MA TRẬN

**Nói:**
> "Sau khi cấp phát, ta đọc dữ liệu từ file vào các ma trận."

### Code đọc ma trận Allocation:
```c
    // Đọc ma trận Allocation (N x M)
    for(int i = 0; i < N; i++) {
        A[i] = (int*)xcalloc(M, sizeof(int));  // Cấp phát hàng i
        for(int j = 0; j < M; j++) {
            if(fscanf(f, "%d", &A[i][j]) != 1) { 
                fprintf(stderr, "Bad A\n"); 
                return 2; 
            }
        }
    }
```

### Code đọc ma trận Need:
```c
    // Đọc ma trận Need (N x M)
    for(int i = 0; i < N; i++) {
        Need[i] = (int*)xcalloc(M, sizeof(int));
        for(int j = 0; j < M; j++) {
            if(fscanf(f, "%d", &Need[i][j]) != 1) { 
                fprintf(stderr, "Bad Need\n"); 
                return 2; 
            }
        }
    }
```

### Code đọc vector Available:
```c
    // Đọc vector Available (M phần tử)
    for(int j = 0; j < M; j++) {
        if(fscanf(f, "%d", &Avail[j]) != 1) { 
            fprintf(stderr, "Bad Available\n"); 
            return 2; 
        }
    }
    fclose(f);
```

---

## 🎯 SLIDE 7: CODE - PHẦN 4: KHỞI TẠO THUẬT TOÁN

**Nói:**
> "Bây giờ đến phần quan trọng - thuật toán phát hiện deadlock. Đầu tiên là khởi tạo."

### Code:
```c
    // Work = bản sao của Available, sẽ được cập nhật khi tiến trình hoàn thành
    int* Work = (int*)xcalloc(M, sizeof(int));
    
    // Finish[i] = true nếu tiến trình i có thể hoàn thành
    bool* Finish = (bool*)xcalloc(N, sizeof(bool));
    
    // Khởi tạo Work = Available
    for(int j = 0; j < M; j++) 
        Work[j] = Avail[j];
```

### Ý nghĩa:
| Biến | Mô tả |
|------|-------|
| `Work[j]` | Tài nguyên loại j **hiện có thể dùng** (ban đầu = Available) |
| `Finish[i]` | `true` nếu Pi **có thể hoàn thành** (ban đầu = `false`) |

### Trạng thái ban đầu:
```
Work = [1, 1]        (copy từ Available)
Finish = [false, false, false]  (chưa ai hoàn thành)
```

---

## 🎯 SLIDE 8: CODE - PHẦN 5: THUẬT TOÁN CHÍNH ⭐

**Nói:**
> "Đây là phần cốt lõi của thuật toán - vòng lặp tìm tiến trình có thể hoàn thành."

### Code:
```c
    bool progress = true;  // Có tiến triển trong vòng lặp này không?
    
    while(progress) {
        progress = false;  // Giả sử không có tiến triển
        
        // Duyệt qua tất cả tiến trình
        for(int i = 0; i < N; i++) {
            // Bỏ qua nếu đã hoàn thành
            if(Finish[i]) continue;
            
            // Kiểm tra: Need[i] <= Work ?
            bool ok = true;
            for(int j = 0; j < M; j++) {
                if(Need[i][j] > Work[j]) { 
                    ok = false; 
                    break; 
                }
            }
            
            // Nếu đủ tài nguyên → tiến trình có thể hoàn thành
            if(ok) {
                // Thu hồi tài nguyên: Work += Allocation[i]
                for(int j = 0; j < M; j++) 
                    Work[j] += A[i][j];
                
                Finish[i] = true;   // Đánh dấu hoàn thành
                progress = true;     // Có tiến triển!
            }
        }
    }
```

### Giải thích thuật toán:
```
┌─────────────────────────────────────────────────────────┐
│  WHILE (còn tiến triển):                                │
│    FOR mỗi tiến trình Pi chưa hoàn thành:              │
│      IF Need[i] <= Work:          // Đủ tài nguyên?     │
│        Work = Work + Allocation[i] // Thu hồi          │
│        Finish[i] = true           // Đánh dấu xong     │
│        progress = true            // Có tiến triển!    │
└─────────────────────────────────────────────────────────┘
```

---

## 🎯 SLIDE 9: MINH HỌA THUẬT TOÁN - VÍ DỤ 1 (NO DEADLOCK)

**Nói:**
> "Hãy xem ví dụ cụ thể để hiểu cách thuật toán hoạt động."

### Input:
```
N=3, M=2

Allocation:       Need:           Available:
     R0 R1            R0 R1           R0 R1
P0 [ 1  0 ]      P0 [ 0  1 ]        [ 1  1 ]
P1 [ 0  1 ]      P1 [ 1  0 ]
P2 [ 0  0 ]      P2 [ 0  0 ]
```

### Mô phỏng từng bước:

**Bước 0 - Khởi tạo:**
```
Work = [1, 1]
Finish = [F, F, F]
```

**Vòng lặp 1:**
```
P0: Need[0]=[0,1] <= Work=[1,1]? ✅ YES
    → Work = [1,1] + [1,0] = [2,1]
    → Finish = [T, F, F]
    
P1: Need[1]=[1,0] <= Work=[2,1]? ✅ YES
    → Work = [2,1] + [0,1] = [2,2]
    → Finish = [T, T, F]
    
P2: Need[2]=[0,0] <= Work=[2,2]? ✅ YES
    → Work = [2,2] + [0,0] = [2,2]
    → Finish = [T, T, T]
```

**Kết quả:** Tất cả `Finish[i] = true` → **NO DEADLOCK** ✅

---

## 🎯 SLIDE 10: MINH HỌA THUẬT TOÁN - VÍ DỤ 2 (DEADLOCK)

**Nói:**
> "Bây giờ xem trường hợp có deadlock."

### Input (05_dead_none_can_start.in):
```
N=2, M=2

Allocation:       Need:           Available:
     R0 R1            R0 R1           R0 R1
P0 [ 1  0 ]      P0 [ 1  0 ]        [ 0  0 ]  ← Không còn tài nguyên rảnh!
P1 [ 0  1 ]      P1 [ 0  1 ]
```

### Mô phỏng:

**Bước 0 - Khởi tạo:**
```
Work = [0, 0]        ← Không có tài nguyên rảnh
Finish = [F, F]
```

**Vòng lặp 1:**
```
P0: Need[0]=[1,0] <= Work=[0,0]? 
    1 > 0 → ❌ NO (không đủ R0)
    
P1: Need[1]=[0,1] <= Work=[0,0]? 
    1 > 0 → ❌ NO (không đủ R1)
```

**Vòng lặp 2:**
```
progress = false → Thoát vòng lặp
Finish = [F, F] → Cả P0 và P1 đều không thể hoàn thành
```

**Kết quả:** **DEADLOCK on 2 process(es): P0 P1** ❌

---

## 🎯 SLIDE 11: CODE - PHẦN 6: XÁC ĐỊNH KẾT QUẢ

**Nói:**
> "Sau khi vòng lặp kết thúc, ta kiểm tra những tiến trình nào không thể hoàn thành."

### Code:
```c
    // Đếm và lưu các tiến trình bị deadlock
    int* dead = (int*)xcalloc(N, sizeof(int));
    int K = 0;  // Số tiến trình deadlock
    
    for(int i = 0; i < N; i++) {
        if(!Finish[i])      // Tiến trình không thể hoàn thành
            dead[K++] = i;   // Thêm vào danh sách deadlock
    }
```

### Giải thích:
```
Sau vòng lặp:
- Finish[i] = true  → Pi có thể hoàn thành ✅
- Finish[i] = false → Pi bị DEADLOCK ❌

Mảng dead[] chứa index các tiến trình deadlock:
  dead = [0, 3, 4]  → P0, P3, P4 bị deadlock
  K = 3             → Có 3 tiến trình deadlock
```

---

## 🎯 SLIDE 12: CODE - PHẦN 7: IN KẾT QUẢ

**Nói:**
> "Cuối cùng là in kết quả và giải phóng bộ nhớ."

### Code in kết quả:
```c
    if(K == 0) {
        // Tất cả tiến trình đều có thể hoàn thành
        puts("NO DEADLOCK");
    } else {
        // Có K tiến trình bị deadlock
        printf("DEADLOCK on %d process(es):", K);
        for(int t = 0; t < K; t++) 
            printf(" P%d", dead[t]);
        putchar('\n');
    }
```

### Code giải phóng bộ nhớ:
```c
    // Giải phóng ma trận
    for(int i = 0; i < N; i++) { 
        free(A[i]); 
        free(Need[i]); 
    }
    free(A); 
    free(Need); 
    
    // Giải phóng vector
    free(Avail); 
    free(Work); 
    free(Finish); 
    free(dead);
    
    return 0;
}
```

### Output mẫu:
```
$ ./detect_matrix test_ok.in
NO DEADLOCK

$ ./detect_matrix test_dead.in
DEADLOCK on 2 process(es): P0 P1
```

---

## 🎯 SLIDE 13: THUẬT TOÁN - PSEUDOCODE TỔNG HỢP

**Nói:**
> "Đây là pseudocode tóm tắt toàn bộ thuật toán."

### Pseudocode:
```
ALGORITHM Deadlock_Detection_Matrix(Allocation, Need, Available, N, M):

    // Bước 1: Khởi tạo
    Work ← Available
    Finish[0..N-1] ← false
    
    // Bước 2: Vòng lặp chính
    REPEAT:
        progress ← false
        
        FOR i ← 0 TO N-1:
            IF Finish[i] = false AND Need[i] ≤ Work:
                Work ← Work + Allocation[i]
                Finish[i] ← true
                progress ← true
                
    UNTIL progress = false
    
    // Bước 3: Xác định kết quả
    deadlocked ← {i : Finish[i] = false}
    
    IF deadlocked = ∅:
        RETURN "NO DEADLOCK"
    ELSE:
        RETURN "DEADLOCK on processes: " + deadlocked
```

---

## 🎯 SLIDE 14: SƠ ĐỒ LUỒNG THUẬT TOÁN

**Nói:**
> "Đây là sơ đồ luồng trực quan của thuật toán."

```
                    ┌─────────────┐
                    │   START     │
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
                    │ Đọc input:  │
                    │ A, Need,    │
                    │ Available   │
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
                    │Work = Avail │
                    │Finish = [F] │
                    └──────┬──────┘
                           │
           ┌───────────────┼───────────────┐
           │               ▼               │
           │      ┌───────────────┐        │
           │      │  progress =   │        │
           │      │    false      │        │
           │      └───────┬───────┘        │
           │              │                │
           │              ▼                │
           │      ┌───────────────┐        │
           │      │ FOR each Pi   │        │
           │      │ not finished  │        │
           │      └───────┬───────┘        │
           │              │                │
           │              ▼                │
           │      ┌───────────────┐        │
           │      │Need[i]<=Work? │        │
           │      └───┬───────┬───┘        │
           │          │       │            │
           │         YES      NO           │
           │          │       │            │
           │          ▼       │            │
           │   ┌────────────┐ │            │
           │   │Work += A[i]│ │            │
           │   │Finish[i]=T │ │            │
           │   │progress=T  │ │            │
           │   └─────┬──────┘ │            │
           │         │        │            │
           │         └───┬────┘            │
           │             ▼                 │
           │      ┌───────────────┐        │
           │      │  progress?    │        │
           │      └───┬───────┬───┘        │
           │         YES      NO           │
           │          │       │            │
           └──────────┘       │            
                              ▼            
                    ┌─────────────────┐    
                    │ Đếm Pi chưa     │    
                    │ finish → dead[] │    
                    └────────┬────────┘    
                             │             
                             ▼             
                    ┌─────────────────┐    
                    │  K == 0 ?       │    
                    └───┬─────────┬───┘    
                       YES        NO       
                        │          │       
                        ▼          ▼       
               ┌──────────┐  ┌───────────┐ 
               │   NO     │  │ DEADLOCK  │ 
               │ DEADLOCK │  │ on K      │ 
               └──────────┘  │ process   │ 
                             └───────────┘ 
```

---

## 🎯 SLIDE 15: ĐỘ PHỨC TẠP VÀ PHÂN TÍCH

**Nói:**
> "Hãy phân tích độ phức tạp của thuật toán."

### Độ phức tạp thời gian:

| Phần | Độ phức tạp | Giải thích |
|------|-------------|------------|
| Đọc input | O(N×M) | Đọc 2 ma trận N×M |
| Khởi tạo | O(N + M) | Khởi tạo Work, Finish |
| Vòng lặp chính | **O(N² × M)** | Tệ nhất: N vòng lặp, mỗi vòng duyệt N tiến trình, so sánh M tài nguyên |
| In kết quả | O(N) | Duyệt mảng Finish |
| **Tổng cộng** | **O(N² × M)** | |

### Độ phức tạp không gian:
```
- Ma trận A, Need: O(N × M)
- Vector Work, Avail: O(M)  
- Vector Finish, dead: O(N)
- Tổng: O(N × M)
```

### Phân tích vòng lặp:
```
Worst case: Mỗi vòng lặp chỉ 1 tiến trình hoàn thành
  → Cần N vòng lặp
  → Mỗi vòng duyệt N tiến trình
  → Mỗi tiến trình so sánh M giá trị
  → O(N × N × M) = O(N² × M)

Best case: Tất cả hoàn thành trong 1 vòng
  → O(N × M)
```

---

## 🎯 SLIDE 16: SO SÁNH VỚI PHƯƠNG PHÁP WFG

**Nói:**
> "Hãy so sánh hai phương pháp phát hiện deadlock."

### Bảng so sánh:

| Tiêu chí | Matrix (Banker's) | WFG (Wait-For Graph) |
|----------|-------------------|----------------------|
| **Loại tài nguyên** | Multi-instance ✅ | Single-instance only |
| **Độ phức tạp** | O(N² × M) | O(N + E) |
| **Thông tin cần** | Allocation, Need, Available | Đồ thị chờ |
| **Kết quả** | Danh sách tiến trình deadlock | Chu trình cụ thể |
| **Áp dụng** | Hệ thống phức tạp | Hệ thống đơn giản |

### Khi nào dùng phương pháp nào?

**Dùng Matrix khi:**
- Mỗi loại tài nguyên có **nhiều instance** (VD: 5 máy in, 3 CPU)
- Cần biết **danh sách** tiến trình deadlock
- Biết trước **Max need** của mỗi tiến trình

**Dùng WFG khi:**
- Mỗi loại tài nguyên chỉ có **1 instance** (VD: 1 mutex, 1 file)
- Cần biết **chu trình chờ** cụ thể
- Đồ thị chờ đã có sẵn

---

## 🎯 SLIDE 17: CÁC TEST CASES

**Nói:**
> "Cuối cùng, xem một số test cases để hiểu rõ hơn."

### Test 1: OK - Simple (01_ok_simple.in)
```
Input:                    Output:
2 2                       NO DEADLOCK
0 1    ← A
1 0
0 0    ← Need (không cần gì)
0 0
1 1    ← Available

Giải thích: Cả P0, P1 đều không cần thêm gì → hoàn thành ngay
```

### Test 2: DEADLOCK - None can start (05_dead_none_can_start.in)
```
Input:                    Output:
2 2                       DEADLOCK on 2 process(es): P0 P1
1 0    ← A (P0 giữ R0)
0 1    ← A (P1 giữ R1)
1 0    ← Need (P0 cần R0)
0 1    ← Need (P1 cần R1)
0 0    ← Available = 0

Giải thích: Available = [0,0], không ai có thể bắt đầu
```

### Test 3: DEADLOCK - Cycle-like (06_dead_cycle_like.in)
```
P0 giữ R0, cần R1
P1 giữ R1, cần R0
Available = [0, 0]

→ P0 chờ P1 giải phóng R1
→ P1 chờ P0 giải phóng R0
→ Deadlock kiểu chu trình!
```

---

## 🎯 SLIDE 18: TỔNG KẾT

**Nói:**
> "Tóm tắt lại những điểm chính của thuật toán Matrix."

### Các bước chính:
```
1️⃣ ĐỌC INPUT: Allocation, Need, Available

2️⃣ KHỞI TẠO: Work = Available, Finish = false

3️⃣ VÒNG LẶP:
   - Tìm Pi: Finish[i]=false VÀ Need[i] <= Work
   - Nếu tìm thấy: Work += A[i], Finish[i] = true
   - Lặp đến khi không còn tiến triển

4️⃣ KẾT QUẢ:
   - Finish[i] = false → Pi bị deadlock
```

### Ưu điểm:
✅ Hoạt động với multi-instance resources  
✅ Cho biết tất cả tiến trình bị deadlock  
✅ Thuật toán đơn giản, dễ cài đặt  

### Nhược điểm:
❌ Độ phức tạp O(N² × M) - cao hơn WFG  
❌ Cần biết trước Max need (không phải lúc nào cũng có)  
❌ Chỉ phát hiện, không chỉ ra nguyên nhân cụ thể  

---

## 🤔 CÂU HỎI THƯỜNG GẶP

**Q1: Tại sao gọi là "Banker's Algorithm"?**
> A: Giống ngân hàng cho vay tiền - chỉ cho vay nếu chắc chắn khách có thể trả. Ở đây, chỉ "cho" tài nguyên nếu chắc chắn tiến trình có thể hoàn thành.

**Q2: Work khác gì Available?**
> A: Available là trạng thái ban đầu, Work thay đổi trong quá trình mô phỏng khi tiến trình "giả lập hoàn thành" và thu hồi tài nguyên.

**Q3: Tại sao cần vòng lặp while(progress)?**
> A: Vì một tiến trình hoàn thành có thể giải phóng tài nguyên cho tiến trình khác. Cần lặp đến khi không còn ai có thể tiến triển.

**Q4: Thuật toán này có thể dùng để TRÁNH deadlock không?**
> A: Có! Phiên bản gốc của Banker's Algorithm dùng để TRÁNH deadlock bằng cách kiểm tra trước khi cấp phát.

---

## 📚 TÀI LIỆU THAM KHẢO

1. **Operating System Concepts** - Silberschatz, Galvin, Gagne - Chapter 8: Deadlocks
2. **Modern Operating Systems** - Andrew S. Tanenbaum - Section 6.4: Deadlock Detection and Recovery
3. **Original Paper**: Dijkstra, E.W. (1965) - "Cooperating Sequential Processes"

---

**🎯 CẢM ƠN MỌI NGƯỜI ĐÃ LẮNG NGHE!**

*Có câu hỏi nào không ạ?*
