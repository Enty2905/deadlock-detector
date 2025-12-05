# 📊 SCRIPT THUYẾT TRÌNH: PHÁT HIỆN DEADLOCK BẰNG WAIT-FOR GRAPH (WFG)

---

## 🎯 SLIDE 1: GIỚI THIỆU TỔNG QUAN

**Nói:**
> "Xin chào mọi người! Hôm nay em sẽ trình bày về thuật toán phát hiện Deadlock sử dụng **Wait-For Graph** (Đồ thị chờ). Đây là một trong những phương pháp phổ biến để phát hiện deadlock trong hệ thống."

### Wait-For Graph là gì?
- Là một **đồ thị có hướng** (directed graph)
- Mỗi **đỉnh** (node) đại diện cho một **tiến trình** (Process)
- Một **cạnh** P1 → P2 có nghĩa: "P1 đang **chờ** P2 giải phóng tài nguyên"
- **Deadlock xảy ra** khi và chỉ khi đồ thị **có chu trình** (cycle)

```
    P0 ──→ P1
    ↑       │
    │       ↓
    P3 ←── P2
    
    ⚠️ Chu trình: P0 → P1 → P2 → P3 → P0 = DEADLOCK!
```

---

## 🎯 SLIDE 2: CẤU TRÚC FILE VÀ THƯ VIỆN

**Nói:**
> "Đầu tiên, hãy xem cấu trúc code. File `detect_wfg.c` sử dụng thư viện graph để xây dựng và phân tích đồ thị."

### Code: Header và Includes
```c
#include <stdio.h>      // Đọc/ghi file, in kết quả
#include <stdlib.h>     // malloc, free, atoi
#include <string.h>     // xử lý chuỗi
#include <stdbool.h>    // kiểu bool
#include "graph.h"      // ⭐ THƯ VIỆN ĐỒ THỊ TỰ XÂY DỰNG
```

### Các hàm chính của thư viện graph.h:
| Hàm | Mô tả |
|-----|-------|
| `graph_create()` | Tạo đồ thị rỗng |
| `graph_get_or_add_node(g, label)` | Thêm/lấy node theo nhãn |
| `graph_add_edge(g, u, v)` | Thêm cạnh u → v |
| `graph_find_cycle(g, &cycle, &len)` | ⭐ **Tìm chu trình trong đồ thị** |
| `graph_node_label(g, id)` | Lấy nhãn của node |
| `graph_free(g)` | Giải phóng bộ nhớ |

---

## 🎯 SLIDE 3: HÀM TIỆN ÍCH - CHUYỂN ĐỔI NHÃN

**Nói:**
> "Tiếp theo là các hàm tiện ích để chuyển đổi giữa PID (số nguyên) và nhãn (chuỗi)."

### Code:
```c
// Chuyển PID thành nhãn: 0 → "P0", 5 → "P5"
static void make_label(int pid, char* buf, size_t n) { 
    snprintf(buf, n, "P%d", pid); 
}

// Chuyển nhãn thành PID: "P0" → 0, "P5" → 5
static int label_to_pid(const char* lab) { 
    return (lab && lab[0]=='P') ? atoi(lab+1) : -1; 
}
```

**Giải thích:**
- `make_label(2, buf, 32)` → buf = "P2"
- `label_to_pid("P2")` → 2
- Dùng để ánh xạ giữa **ID trong đồ thị** và **PID của tiến trình**

---

## 🎯 SLIDE 4: HÀM XỬ LÝ CHU TRÌNH - LOẠI BỎ TRÙNG LẶP

**Nói:**
> "Khi tìm được chu trình từ DFS, có thể có các phần tử trùng lặp liền nhau. Hàm này giúp loại bỏ chúng."

### Code: dedup_consecutive
```c
// Input:  [1, 1, 2, 2, 2, 3, 1, 1]
// Output: [1, 2, 3, 1] (loại bỏ trùng liên tiếp)

static size_t dedup_consecutive(const int* a, size_t n, int* out) {
    if(n == 0) return 0;
    
    size_t m = 0;
    out[m++] = a[0];  // Luôn giữ phần tử đầu tiên
    
    for(size_t i = 1; i < n; i++) {
        // Chỉ thêm nếu khác phần tử trước đó
        if(a[i] != out[m-1]) 
            out[m++] = a[i];
    }
    return m;  // Số phần tử sau khi loại bỏ
}
```

**Ví dụ minh họa:**
```
Input:  [P0, P0, P1, P1, P2, P2, P0]
Output: [P0, P1, P2, P0]
         ↑   Chu trình gọn hơn!
```

---

## 🎯 SLIDE 5: HÀM TRÍCH XUẤT CHU TRÌNH ĐƠN

**Nói:**
> "Đây là hàm quan trọng để trích xuất chu trình đơn từ đường đi DFS. Vì DFS có thể trả về đường đi dài với chu trình nằm ở giữa."

### Code: extract_simple_cycle_window
```c
static size_t extract_simple_cycle_window(const int* q, size_t m, int* cyc) {
    if(m == 0) return 0;
    
    // CASE 1: Đầu và cuối giống nhau → đã là chu trình hoàn chỉnh
    // [P0, P1, P2, P0] → [P0, P1, P2]
    if(m >= 2 && q[m-1] == q[0]) { 
        for(size_t i = 0; i < m-1; i++) 
            cyc[i] = q[i];
        return m - 1;
    }
    
    // CASE 2: Tìm cặp (s, e) sao cho q[s] == q[e], e > s
    // [P5, P0, P1, P2, P0, P3] 
    //       ↑           ↑
    //       s=1         e=4  → trích [P0, P1, P2]
    
    size_t s_idx = 0, e_idx = 0;
    bool found = false;
    
    for(size_t s = 0; s < m; s++) {
        for(size_t e = m-1; e > s; e--) {
            if(q[s] == q[e]) {
                s_idx = s; e_idx = e; 
                found = true;
                goto got_pair;  // Nhảy ra khỏi 2 vòng lặp
            }
        }
    }
got_pair:

    if(found) {
        size_t L = 0;
        for(size_t i = s_idx; i < e_idx; i++) 
            cyc[L++] = q[i];
        return L;
    }
    
    // Không tìm thấy → trả về toàn bộ
    for(size_t i = 0; i < m; i++) cyc[i] = q[i];
    return m;
}
```

**Minh họa trực quan:**
```
DFS path: [P5] → [P0] → [P1] → [P2] → [P0] → [P3]
                  ↑                     ↑
                  └── Cùng là P0 ───────┘
                  
Trích xuất: [P0, P1, P2] = Chu trình đơn!
```

---

## 🎯 SLIDE 6: HÀM XOAY CHU TRÌNH - CHUẨN HÓA OUTPUT

**Nói:**
> "Để output nhất quán, ta xoay chu trình sao cho phần tử nhỏ nhất lên đầu."

### Code: rotate_min
```c
// Input:  [P2, P3, P0, P1]
// Output: [P0, P1, P2, P3]  ← P0 nhỏ nhất, đưa lên đầu

static void rotate_min(int* a, size_t n) {
    if(n == 0) return;
    
    // Bước 1: Tìm vị trí phần tử nhỏ nhất
    size_t s = 0;
    for(size_t i = 1; i < n; i++) 
        if(a[i] < a[s]) s = i;
    
    if(s == 0) return;  // Đã ở đầu rồi
    
    // Bước 2: Xoay mảng
    int* tmp = (int*)malloc(sizeof(int) * n);
    for(size_t k = 0; k < n; k++) 
        tmp[k] = a[(s + k) % n];  // Phép modulo để xoay vòng
    
    for(size_t k = 0; k < n; k++) 
        a[k] = tmp[k];
    
    free(tmp);
}
```

**Ví dụ:**
```
Trước: [P2, P0, P1]   (chu trình P2→P0→P1→P2)
Sau:   [P0, P1, P2]   (chu trình P0→P1→P2→P0) ← Cùng chu trình, nhưng gọn hơn!
```

---

## 🎯 SLIDE 7: HÀM MAIN - PHẦN 1: ĐỌC INPUT

**Nói:**
> "Bây giờ đến phần chính - hàm main. Đầu tiên là đọc đồ thị từ file."

### Định dạng file input:
```
N E           ← N: số node, E: số cạnh
u1 v1         ← Cạnh từ P(u1) → P(v1)
u2 v2
...
```

**Ví dụ file input:**
```
4 4           ← 4 tiến trình, 4 cạnh
0 1           ← P0 chờ P1
1 2           ← P1 chờ P2
2 3           ← P2 chờ P3
3 0           ← P3 chờ P0 → TẠO THÀNH CHU TRÌNH!
```

### Code đọc file:
```c
int main(int argc, char** argv) {
    // Kiểm tra tham số
    if(argc < 2) { 
        fprintf(stderr, "Usage: %s <input>\n", argv[0]); 
        return 2; 
    }
    
    // Mở file
    FILE* f = fopen(argv[1], "r");
    if(!f) { perror("fopen"); return 2; }

    // Đọc số đỉnh N và số cạnh E
    int N, E;
    if(fscanf(f, "%d %d", &N, &E) != 2) { 
        fprintf(stderr, "Bad header\n"); 
        fclose(f); 
        return 2; 
    }

    // Cấp phát mảng lưu các cạnh
    int* U = (int*)malloc(sizeof(int) * E);  // Đỉnh nguồn
    int* V = (int*)malloc(sizeof(int) * E);  // Đỉnh đích
    
    // Đọc từng cạnh
    for(int i = 0; i < E; i++) {
        if(fscanf(f, "%d %d", &U[i], &V[i]) != 2) {
            fprintf(stderr, "Bad edge at %d\n", i);
            fclose(f); free(U); free(V); 
            return 2;
        }
    }
    fclose(f);
```

---

## 🎯 SLIDE 8: HÀM MAIN - PHẦN 2: XÂY DỰNG ĐỒ THỊ

**Nói:**
> "Sau khi đọc dữ liệu, ta xây dựng đồ thị Wait-For Graph."

### Code:
```c
    // Tạo đồ thị rỗng
    graph_t* g = graph_create();
    if(!g) { 
        fprintf(stderr, "graph_create failed\n"); 
        free(U); free(V); 
        return 2; 
    }
    
    // Thêm N node với nhãn P0, P1, ..., P(N-1)
    for(int i = 0; i < N; i++) { 
        char lab[32]; 
        make_label(i, lab, sizeof(lab));  // i=0 → "P0"
        graph_get_or_add_node(g, lab); 
    }
    
    // Thêm E cạnh có hướng
    for(int i = 0; i < E; i++) {
        char lu[32], lv[32];
        make_label(U[i], lu, sizeof(lu));  // Nhãn đỉnh nguồn
        make_label(V[i], lv, sizeof(lv));  // Nhãn đỉnh đích
        
        int uid = graph_get_or_add_node(g, lu);
        int vid = graph_get_or_add_node(g, lv);
        
        graph_add_edge(g, uid, vid);  // Thêm cạnh uid → vid
    }
```

**Minh họa:**
```
File input:        Đồ thị tạo ra:
4 4               
0 1                P0 ──→ P1
1 2                       │
2 3                       ↓
3 0                P3 ←── P2
                   │
                   └──→ P0 (tạo chu trình!)
```

---

## 🎯 SLIDE 9: HÀM MAIN - PHẦN 3: TÌM CHU TRÌNH (DEADLOCK)

**Nói:**
> "Đây là bước quan trọng nhất - gọi hàm graph_find_cycle để tìm chu trình."

### Code:
```c
    // Tìm chu trình trong đồ thị
    int* cyc_ids = NULL;   // Mảng chứa ID các node trong chu trình
    size_t L_ids = 0;       // Độ dài chu trình
    
    // ⭐ HÀM QUAN TRỌNG: Dùng DFS để tìm back-edge
    if(!graph_find_cycle(g, &cyc_ids, &L_ids)) {
        // KHÔNG có chu trình = KHÔNG có deadlock
        puts("NO DEADLOCK");
        graph_free(g); 
        free(U); free(V);
        return 0;
    }
    
    // CÓ chu trình → CÓ deadlock
    // Chuyển từ graph ID sang PID
    int* raw = (int*)malloc(sizeof(int) * L_ids);
    for(size_t i = 0; i < L_ids; i++) {
        const char* lab = graph_node_label(g, cyc_ids[i]);
        raw[i] = label_to_pid(lab);  // "P2" → 2
    }
    free(cyc_ids);
```

### Thuật toán tìm chu trình (DFS):
```
1. Duyệt DFS từ mỗi node chưa thăm
2. Đánh màu node:
   - WHITE (0): Chưa thăm
   - GRAY (1): Đang thăm (trong stack DFS)
   - BLACK (2): Đã xong
3. Nếu gặp GRAY node → BACK-EDGE → CÓ CHU TRÌNH!
```

---

## 🎯 SLIDE 10: HÀM MAIN - PHẦN 4: XỬ LÝ VÀ CHUẨN HÓA CHU TRÌNH

**Nói:**
> "Sau khi tìm được chu trình, ta cần xử lý để có output gọn gàng."

### Code:
```c
    // Bước 1: Loại bỏ các PID trùng liên tiếp
    int* q = (int*)malloc(sizeof(int) * L_ids);
    size_t qn = dedup_consecutive(raw, L_ids, q);
    
    // Bước 2: Trích xuất chu trình đơn
    int* cyc = (int*)malloc(sizeof(int) * qn);
    size_t L = extract_simple_cycle_window(q, qn, cyc);
    
    // Bước 3: Loại bỏ trùng lặp một lần nữa (để chắc chắn)
    L = dedup_consecutive(cyc, L, cyc);
    free(raw); 
    free(q);

    // Bước 4: Xoay để phần tử nhỏ nhất lên đầu
    if(L == 0) {
        puts("DEADLOCK");  // Có deadlock nhưng không trích được chu trình
        graph_free(g); free(U); free(V); free(cyc);
        return 0;
    }
    rotate_min(cyc, L);
```

**Ví dụ biến đổi:**
```
raw = [0, 0, 1, 2, 2, 3, 0]
  ↓ dedup_consecutive
q   = [0, 1, 2, 3, 0]
  ↓ extract_simple_cycle_window  
cyc = [0, 1, 2, 3]
  ↓ rotate_min (0 đã nhỏ nhất)
cyc = [0, 1, 2, 3]

Output: "DEADLOCK cycle: P0 P1 P2 P3 P0"
```

---

## 🎯 SLIDE 11: HÀM MAIN - PHẦN 5: KIỂM TRA CHU TRÌNH RIÊNG RẼ

**Nói:**
> "Code còn kiểm tra xem có chu trình khác không thuộc chu trình đã tìm không. Nếu có, chỉ báo DEADLOCK mà không in chi tiết."

### Code:
```c
    // Đánh dấu các node thuộc chu trình đã tìm
    bool* inC = (bool*)calloc((size_t)N, sizeof(bool));
    for(size_t i = 0; i < L; i++) 
        if(cyc[i] >= 0 && cyc[i] < N) 
            inC[cyc[i]] = true;

    // Tạo đồ thị con chứa các node KHÔNG thuộc chu trình
    graph_t* g2 = graph_create();
    
    // Thêm các node không trong chu trình
    for(int i = 0; i < N; i++) 
        if(!inC[i]) { 
            char lab[32]; 
            make_label(i, lab, sizeof(lab)); 
            graph_get_or_add_node(g2, lab); 
        }
    
    // Thêm các cạnh giữa các node không trong chu trình
    for(int i = 0; i < E; i++) {
        if(!inC[U[i]] && !inC[V[i]]) {
            char lu[32], lv[32]; 
            make_label(U[i], lu, sizeof(lu)); 
            make_label(V[i], lv, sizeof(lv));
            int uid = graph_get_or_add_node(g2, lu);
            int vid = graph_get_or_add_node(g2, lv);
            graph_add_edge(g2, uid, vid);
        }
    }
    
    // Kiểm tra xem phần còn lại có chu trình không
    int* cyc2 = NULL; 
    size_t L2 = 0;
    bool has_disjoint = graph_find_cycle(g2, &cyc2, &L2);
```

---

## 🎯 SLIDE 12: HÀM MAIN - PHẦN 6: IN KẾT QUẢ

**Nói:**
> "Cuối cùng là in kết quả và giải phóng bộ nhớ."

### Code:
```c
    if(has_disjoint) {
        // Có chu trình khác → Nhiều deadlock, chỉ báo chung
        puts("DEADLOCK");
    } else {
        // Chỉ có 1 chu trình → In chi tiết
        printf("DEADLOCK cycle: ");
        for(size_t i = 0; i < L; i++) {
            printf("P%d ", cyc[i]);
        }
        printf("P%d\n", cyc[0]);  // Đóng vòng
    }

    // Giải phóng bộ nhớ
    free(cyc); 
    free(inC);
    graph_free(g); 
    free(U); 
    free(V);
    if(cyc2) free(cyc2);
    graph_free(g2);
    
    return 0;
}
```

### Output mẫu:
```
$ ./detect_wfg test1.in
NO DEADLOCK

$ ./detect_wfg test2.in  
DEADLOCK cycle: P0 P1 P2 P3 P0

$ ./detect_wfg test3.in   # Có nhiều chu trình
DEADLOCK
```

---

## 🎯 SLIDE 13: THUẬT TOÁN GRAPH_FIND_CYCLE (BÊN TRONG)

**Nói:**
> "Hãy xem chi tiết thuật toán tìm chu trình dùng DFS."

### Code trong graph.c:
```c
// DFS với 3 màu để phát hiện back-edge
static bool dfs_backedge(const graph_t* g, int u, int* color, int* parent,
                         int* out_u, int* out_v) {
    color[u] = 1;  // GRAY - đang thăm
    
    // Duyệt tất cả hàng xóm của u
    for(size_t i = 0; i < g->adj[u].n; i++) {
        int v = g->adj[u].a[i];
        
        if(color[v] == 0) {  // WHITE - chưa thăm
            parent[v] = u;
            if(dfs_backedge(g, v, color, parent, out_u, out_v)) 
                return true;
        } 
        else if(color[v] == 1) {  // GRAY - back-edge!
            *out_u = u; 
            *out_v = v;
            return true;  // ⭐ TÌM THẤY CHU TRÌNH!
        }
    }
    
    color[u] = 2;  // BLACK - hoàn thành
    return false;
}
```

### Minh họa DFS:
```
     P0 ──→ P1 ──→ P2
            ↑       │
            └───────┘
            back-edge!

DFS từ P0:
1. Visit P0 (gray)
2. Visit P1 (gray) 
3. Visit P2 (gray)
4. P2 có cạnh tới P1, P1 đang gray → BACK-EDGE!
5. Chu trình: P1 → P2 → P1
```

---

## 🎯 SLIDE 14: TỔNG KẾT THUẬT TOÁN

**Nói:**
> "Tóm tắt lại toàn bộ thuật toán phát hiện deadlock bằng WFG."

### Sơ đồ luồng:
```
┌─────────────────┐
│   Đọc file      │
│   input         │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Xây dựng đồ    │
│  thị WFG        │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Tìm chu trình  │──── Không ────→ "NO DEADLOCK"
│  (DFS 3 màu)    │
└────────┬────────┘
         │ Có
         ▼
┌─────────────────┐
│  Xử lý & chuẩn  │
│  hóa chu trình  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Kiểm tra chu    │──── Có thêm ──→ "DEADLOCK"
│ trình khác?     │
└────────┬────────┘
         │ Không
         ▼
┌─────────────────┐
│ In: "DEADLOCK   │
│ cycle: P0 P1.." │
└─────────────────┘
```

---

## 🎯 SLIDE 15: ĐỘ PHỨC TẠP & ƯU NHƯỢC ĐIỂM

### Độ phức tạp:
| Bước | Thời gian | Giải thích |
|------|-----------|------------|
| Đọc file | O(E) | Đọc E cạnh |
| Xây đồ thị | O(N + E) | Thêm N node, E cạnh |
| Tìm chu trình (DFS) | **O(N + E)** | Mỗi node/cạnh thăm 1 lần |
| Xử lý chu trình | O(L²) | L = độ dài chu trình |
| **Tổng** | **O(N + E)** | Rất hiệu quả! |

### Ưu điểm:
✅ Thuật toán đơn giản, dễ hiểu  
✅ Độ phức tạp tuyến tính O(N + E)  
✅ Phát hiện chính xác deadlock  
✅ Có thể chỉ ra chu trình cụ thể  

### Nhược điểm:
❌ Chỉ áp dụng cho single-instance resources  
❌ Cần phải xây dựng lại đồ thị khi hệ thống thay đổi  
❌ Chỉ phát hiện, không tự động giải quyết deadlock  

---

## 🎯 SLIDE 16: DEMO VÀ TEST CASES

**Nói:**
> "Cuối cùng, hãy xem một số test cases."

### Test 1: Có chu trình đơn (3 nodes)
```
Input (03_cycle_2nodes.in):
2 2
0 1
1 0

Output: DEADLOCK cycle: P0 P1 P0
```

### Test 2: Không có chu trình (DAG)
```
Input (07_nocycle_dag.in):
5 4
0 1
0 2
1 3
2 3

Output: NO DEADLOCK
```

### Test 3: Chu trình trong đồ thị lớn
```
Input (05_cycle_in_biggraph.in):
6 7
0 1
1 2
2 0    ← Chu trình P0→P1→P2→P0
3 4
4 5

Output: DEADLOCK cycle: P0 P1 P2 P0
```

---

## 🎤 CÂU HỎI THƯỜNG GẶP

**Q1: Tại sao dùng DFS 3 màu?**
> A: Để phân biệt node đang trong stack DFS (gray) với node đã xong (black). Back-edge chỉ xảy ra khi gặp node gray.

**Q2: Tại sao cần xoay chu trình?**
> A: Để output nhất quán. Ví dụ chu trình [P2,P0,P1] và [P0,P1,P2] là như nhau, ta chuẩn hóa thành [P0,P1,P2].

**Q3: WFG khác gì với Resource Allocation Graph?**
> A: RAG có 2 loại node (Process và Resource), WFG chỉ có Process. WFG đơn giản hơn nhưng chỉ dùng được cho single-instance resources.

---

## 📚 TÀI LIỆU THAM KHẢO

1. Operating System Concepts - Silberschatz, Chapter 8: Deadlocks
2. Graph Algorithms - DFS and Cycle Detection
3. CLRS - Introduction to Algorithms, Chapter 22: Elementary Graph Algorithms

---

**🎯 CẢM ƠN MỌI NGƯỜI ĐÃ LẮNG NGHE!**

*Có câu hỏi nào không ạ?*
