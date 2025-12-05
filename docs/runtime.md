# 📊 SCRIPT THUYẾT TRÌNH: PHÁT HIỆN DEADLOCK RUNTIME VỚI LIBDD

---

## 🎯 SLIDE 1: GIỚI THIỆU TỔNG QUAN

**Nói:**
> "Xin chào mọi người! Phần này em sẽ trình bày về **libdd** - một thư viện **Runtime Deadlock Detection**. Đây là phương pháp phát hiện deadlock **trong lúc chương trình đang chạy**, khác với 2 phương pháp trước (WFG và Matrix) là phân tích offline."

### Runtime Detection là gì?
- Phát hiện deadlock **NGAY KHI** nó xảy ra (hoặc sắp xảy ra)
- **Hook** (chặn) các hàm pthread_mutex_lock/unlock
- Xây dựng **Wait-For Graph động** trong runtime
- Kiểm tra chu trình **mỗi khi** có thread chờ mutex

### Ưu điểm chính:
```
✅ Không cần biết trước cấu trúc chương trình
✅ Phát hiện deadlock theo thời gian thực
✅ Có thể tích hợp vào bất kỳ chương trình nào (LD_PRELOAD)
✅ Không cần sửa source code của ứng dụng
```

---

## 🎯 SLIDE 2: KIẾN TRÚC TỔNG QUAN

**Nói:**
> "Hãy xem kiến trúc tổng quan của libdd."

### Sơ đồ hoạt động:
```
┌─────────────────────────────────────────────────────────────┐
│                    APPLICATION                               │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                      │
│  │ Thread1 │  │ Thread2 │  │ Thread3 │                      │
│  └────┬────┘  └────┬────┘  └────┬────┘                      │
│       │            │            │                            │
│       ▼            ▼            ▼                            │
│  pthread_mutex_lock() / unlock()                             │
└──────────────────────┬──────────────────────────────────────┘
                       │ LD_PRELOAD
                       ▼
┌──────────────────────────────────────────────────────────────┐
│                      LIBDD (Hook Layer)                       │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐  │
│  │pthread_mutex_  │  │pthread_mutex_  │  │pthread_mutex_  │  │
│  │    lock()      │  │   unlock()     │  │   trylock()    │  │
│  └───────┬────────┘  └───────┬────────┘  └───────┬────────┘  │
│          │                   │                   │           │
│          └───────────────────┼───────────────────┘           │
│                              ▼                               │
│                    ┌─────────────────┐                       │
│                    │   WAIT-FOR      │                       │
│                    │   GRAPH (WFG)   │                       │
│                    │   ┌───┐  ┌───┐  │                       │
│                    │   │T1 │→→│M_A│  │  → Kiểm tra chu trình │
│                    │   └───┘  └─┬─┘  │                       │
│                    │            ↓    │                       │
│                    │         ┌───┐   │                       │
│                    │         │T2 │   │                       │
│                    │         └───┘   │                       │
│                    └─────────────────┘                       │
└──────────────────────────────────────────────────────────────┘
                       │
                       ▼
              ┌─────────────────┐
              │  REAL PTHREAD   │
              │    FUNCTIONS    │
              └─────────────────┘
```

---

## 🎯 SLIDE 3: CƠ CHẾ LD_PRELOAD

**Nói:**
> "libdd sử dụng cơ chế **LD_PRELOAD** của Linux để hook các hàm pthread."

### LD_PRELOAD là gì?
```bash
# Cách chạy bình thường:
./my_app

# Cách chạy với libdd:
LD_PRELOAD=./libdd.so ./my_app
```

### Cơ chế hoạt động:
```
1. Khi app gọi pthread_mutex_lock()
   
2. Linker tìm symbol "pthread_mutex_lock" 
   
3. LD_PRELOAD ưu tiên tìm trong libdd.so TRƯỚC
   
4. libdd.so có hàm pthread_mutex_lock() → được gọi!
   
5. libdd xử lý logic → gọi hàm THẬT qua dlsym(RTLD_NEXT)
```

### Code lấy hàm thật:
```c
#include <dlfcn.h>

// Con trỏ tới hàm pthread thật
static int (*real_mutex_lock)(pthread_mutex_t*) = NULL;

// Macro load symbol an toàn
#define LOAD_SYM(dst, name) do {                         \
    void* __p = dlsym(RTLD_NEXT, (name));                \
    if (!__p) { /* handle error */ abort(); }            \
    *(void**)&(dst) = __p;                               \
} while(0)

// Trong constructor:
LOAD_SYM(real_mutex_lock, "pthread_mutex_lock");
```

---

## 🎯 SLIDE 4: CẤU TRÚC DỮ LIỆU

**Nói:**
> "libdd sử dụng 2 cấu trúc dữ liệu chính: đồ thị WFG và bảng mutex-owner."

### 1. Wait-For Graph (g_wfg):
```c
static graph_t* g_wfg = NULL;  // Đồ thị có nhãn node
```

**Các loại node trong WFG:**
| Node | Ý nghĩa | Ví dụ |
|------|---------|-------|
| `T<tid>` | Thread với ID = tid | T12345 |
| `M<addr>` | Mutex tại địa chỉ addr | M0x7fff5a3b |

**Các loại cạnh:**
| Cạnh | Ý nghĩa | Khi nào thêm |
|------|---------|-------------|
| T → M | Thread **đang CHỜ** mutex | Khi gọi lock() và mutex bận |
| M → T | Mutex **thuộc về** thread | Khi thread chiếm được mutex |

### 2. Bảng Mutex-Owner (g_tab):
```c
// Theo dõi ai đang sở hữu mutex nào
typedef struct { 
    pthread_mutex_t* m;      // Con trỏ tới mutex
    unsigned long owner;     // Thread ID của owner (0 nếu free)
    int count;               // Số lần lock (recursive mutex)
} mrec_t;

static mrec_t* g_tab = NULL;   // Mảng động
static size_t g_sz = 0;        // Số phần tử hiện tại
static size_t g_cap = 0;       // Capacity
```

---

## 🎯 SLIDE 5: REENTRANCY GUARD

**Nói:**
> "Một vấn đề quan trọng là **tránh đệ quy vô hạn** khi hook gọi lại chính nó."

### Vấn đề:
```c
// Hook của chúng ta
int pthread_mutex_lock(pthread_mutex_t* m) {
    mu_lock();  // Gọi pthread_mutex_lock → GỌI LẠI CHÍNH NÓ!
    // ...
}
```

### Giải pháp 1: Reentrancy Guard
```c
// Thread-local variable để đánh dấu "đang trong hook"
static __thread int dd_inhook = 0;

static inline void dd_enter(void) { dd_inhook++; }
static inline void dd_leave(void) { dd_inhook--; }
static inline int  dd_in(void)    { return dd_inhook; }

// Trong hook:
int pthread_mutex_lock(pthread_mutex_t* m) {
    if (dd_in()) {
        return real_mutex_lock(m);  // Bypass hook
    }
    dd_enter();
    // ... logic ...
    dd_leave();
}
```

### Giải pháp 2: Dùng hàm thật cho internal mutex
```c
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;

// Lock internal mutex bằng hàm THẬT
static inline void mu_lock(void) {
    real_mutex_lock(&g_mu);      // Không qua hook!
}

static inline void mu_unlock(void) {
    real_mutex_unlock(&g_mu);    // Không qua hook!
}
```

---

## 🎯 SLIDE 6: HÀM KHỞI TẠO VÀ HỦY

**Nói:**
> "libdd sử dụng constructor và destructor để tự động khởi tạo khi load."

### Constructor (chạy khi library được load):
```c
__attribute__((constructor))
static void dd_init(void) {
    // 1. Load các hàm pthread thật
    LOAD_SYM(real_mutex_lock,    "pthread_mutex_lock");
    LOAD_SYM(real_mutex_trylock, "pthread_mutex_trylock");
    LOAD_SYM(real_mutex_unlock,  "pthread_mutex_unlock");
    
    // 2. Đọc log level từ environment variable
    const char* lv = getenv("DD_LOG_LEVEL"); 
    if (lv && *lv) g_log = atoi(lv);
    
    // 3. Tạo đồ thị WFG rỗng
    g_wfg = graph_create(); 
    if (!g_wfg) { 
        fprintf(stderr, "[libdd] graph_create failed\n"); 
        abort(); 
    }
    
    dd_log(2, "[libdd] init ok (log=%d)\n", g_log);
}
```

### Destructor (chạy khi library được unload):
```c
__attribute__((destructor))
static void dd_fini(void) {
    // Giải phóng bảng mutex-owner
    free(g_tab); 
    g_tab = NULL; 
    g_sz = g_cap = 0;
    
    // Giải phóng đồ thị
    graph_free(g_wfg); 
    g_wfg = NULL;
}
```

### Logging levels:
```bash
# Level 0: Tắt log
DD_LOG_LEVEL=0 LD_PRELOAD=./libdd.so ./app

# Level 1: Chỉ log khi phát hiện chu trình (mặc định)
DD_LOG_LEVEL=1 LD_PRELOAD=./libdd.so ./app

# Level 2: Log chi tiết mọi thao tác
DD_LOG_LEVEL=2 LD_PRELOAD=./libdd.so ./app
```

---

## 🎯 SLIDE 7: HOOK PTHREAD_MUTEX_LOCK - PHẦN 1

**Nói:**
> "Đây là hook quan trọng nhất. Hãy phân tích từng bước."

### Code - Phần đầu: Kiểm tra và thêm cạnh chờ
```c
int pthread_mutex_lock(pthread_mutex_t* m) {
    unsigned long me = tid_self();  // Thread ID hiện tại
    int me_node = -1, m_node = -1;
    int added_wait_edge = 0;

    /* ====== BƯỚC 1: Kiểm tra và thêm cạnh T→M nếu cần ====== */
    mu_lock();  // Lock internal mutex (dùng hàm thật)
    
    mrec_t* r = tab_get_or_add(m);     // Lấy/tạo record cho mutex
    unsigned long owner = r->owner;     // Ai đang giữ mutex?
    
    me_node = node_of_thread(me);       // Node "T<my_tid>"
    m_node  = node_of_mutex(m);         // Node "M<addr>"
    
    // Chỉ thêm cạnh T→M khi mutex do thread KHÁC giữ
    if (owner != 0 && owner != me) {
        graph_add_edge(g_wfg, me_node, m_node);  // T → M (đang CHỜ)
        added_wait_edge = 1;
        
        dd_log(2, "[libdd] wait T%lu -> M%p (owner=T%lu)\n", 
               me, (void*)m, owner);
               
        log_cycle_if_any();  // ⭐ KIỂM TRA DEADLOCK!
    } else {
        dd_log(2, "[libdd] fastpath lock M%p by T%lu\n", (void*)m, me);
    }
    
    mu_unlock();
```

### Minh họa:
```
Thread 1 (T1) gọi lock(M_A), M_A đang do T2 giữ:

Trước:                    Sau:
  T1      T2               T1 ────→ M_A
          │                          │
          ▼                          ▼
         M_A                        T2

Thêm cạnh: T1 → M_A (T1 đang chờ M_A)
```

---

## 🎯 SLIDE 8: HOOK PTHREAD_MUTEX_LOCK - PHẦN 2

**Nói:**
> "Tiếp theo là gọi hàm lock thật và xử lý sau khi lock thành công."

### Code - Phần sau: Gọi lock thật và cập nhật đồ thị
```c
    /* ====== BƯỚC 2: Gọi lock thật (có thể chờ ở đây) ====== */
    int rc = real_mutex_lock(m);
    
    // Nếu lock thất bại, gỡ cạnh đã thêm
    if (rc != 0) {
        mu_lock();
        if (added_wait_edge) 
            graph_remove_edge(g_wfg, me_node, m_node);
        mu_unlock();
        return rc;
    }

    /* ====== BƯỚC 3: Lock thành công - cập nhật đồ thị ====== */
    mu_lock();
    
    // Gỡ cạnh T→M (không còn chờ nữa)
    if (added_wait_edge) 
        graph_remove_edge(g_wfg, me_node, m_node);
    
    // Cập nhật owner trong bảng
    r = tab_get_or_add(m);
    if (r->owner == me) 
        r->count += 1;           // Recursive lock
    else { 
        r->owner = me; 
        r->count = 1; 
    }
    
    // Thêm cạnh M→T (mutex thuộc về thread này)
    graph_add_edge(g_wfg, m_node, me_node);
    
    dd_log(2, "[libdd] acquired M%p -> T%lu (count=%d)\n", 
           (void*)m, me, r->count);
           
    mu_unlock();
    return 0;
}
```

### Minh họa trạng thái sau khi lock thành công:
```
Trước (đang chờ):         Sau (đã có lock):
T1 ────→ M_A              M_A ────→ T1
          │                
          ▼               (T1 sở hữu M_A)
         T2               
                          
Gỡ: T1 → M_A              
Thêm: M_A → T1
```

---

## 🎯 SLIDE 9: HOOK PTHREAD_MUTEX_UNLOCK

**Nói:**
> "Hàm unlock đơn giản hơn - chỉ cần gỡ cạnh sở hữu."

### Code:
```c
int pthread_mutex_unlock(pthread_mutex_t* m) {
    // Gọi unlock thật TRƯỚC
    int rc = real_mutex_unlock(m);
    if (rc != 0) return rc;

    mu_lock();
    
    mrec_t* r = tab_get_or_add(m);
    unsigned long me = tid_self();
    int me_node = node_of_thread(me);
    int m_node  = node_of_mutex(m);

    // Gỡ cạnh M→T (không còn sở hữu)
    graph_remove_edge(g_wfg, m_node, me_node);

    // Cập nhật bảng owner
    if (r->owner == me) {
        if (r->count > 1) 
            r->count -= 1;      // Recursive: giảm count
        else { 
            r->count = 0; 
            r->owner = 0;       // Mutex free
        }
    } else {
        // Warning: unlock bởi thread không phải owner
        dd_log(2, "[libdd] warn: unlock by non-owner T%lu on M%p\n",
               me, (void*)m);
    }
    
    dd_log(2, "[libdd] released M%p by T%lu\n", (void*)m, me);
    
    mu_unlock();
    return 0;
}
```

### Minh họa:
```
Trước unlock:             Sau unlock:
M_A ────→ T1              (không còn cạnh)
                          
owner = T1                owner = 0 (free)
count = 1                 count = 0
```

---

## 🎯 SLIDE 10: HOOK PTHREAD_MUTEX_TRYLOCK

**Nói:**
> "trylock khác lock ở chỗ nó KHÔNG CHỜ. Nếu mutex bận, trả về ngay."

### Code:
```c
int pthread_mutex_trylock(pthread_mutex_t* m) {
    // trylock KHÔNG chờ → không tạo cạnh T→M
    int rc = real_mutex_trylock(m);
    
    if (rc == 0) {
        // Thành công: cập nhật như lock()
        unsigned long me = tid_self();
        
        mu_lock();
        mrec_t* r = tab_get_or_add(m);
        r->owner = me; 
        r->count += 1;
        
        // Thêm cạnh sở hữu M→T
        graph_add_edge(g_wfg, node_of_mutex(m), node_of_thread(me));
        
        dd_log(2, "[libdd] trylock OK: M%p -> T%lu\n", (void*)m, me);
        mu_unlock();
        
    } else {
        // Thất bại: không làm gì (không chờ = không có cạnh)
        dd_log(2, "[libdd] trylock BUSY: M%p\n", (void*)m);
    }
    
    return rc;
}
```

### So sánh lock() vs trylock():
| Hành vi | lock() | trylock() |
|---------|--------|-----------|
| Mutex free | Chiếm, thêm M→T | Chiếm, thêm M→T |
| Mutex bận | **CHỜ**, thêm T→M | **TRẢ VỀ NGAY**, không thêm cạnh |
| Có thể deadlock? | **CÓ** | **KHÔNG** |

---

## 🎯 SLIDE 11: KIỂM TRA CHU TRÌNH - PHÁT HIỆN DEADLOCK

**Nói:**
> "Mỗi khi thêm cạnh T→M, libdd kiểm tra chu trình ngay lập tức."

### Code:
```c
// Debounce: tránh spam log (200ms giữa các lần báo)
static long long last_cycle_ms = 0;

static void log_cycle_if_any(void) {
    int* cyc = NULL; 
    size_t L = 0;
    
    // Kiểm tra chu trình trong đồ thị
    if (graph_find_cycle(g_wfg, &cyc, &L)) {
        long long t = now_ms();
        
        // Debounce: chỉ log nếu cách lần trước >= 200ms
        if (g_log >= 1 && t - last_cycle_ms >= 200) {
            last_cycle_ms = t;
            
            // In chu trình ra stderr
            fprintf(stderr, "[libdd] DEADLOCK cycle: ");
            for (size_t i = 0; i < L; i++) {
                fprintf(stderr, "%s%s", 
                        graph_node_label(g_wfg, cyc[i]), 
                        (i+1 < L) ? " " : "");
            }
            fputc('\n', stderr);
        }
        free(cyc);
    }
}
```

### Ví dụ output:
```
[libdd] DEADLOCK cycle: T12345 M0x7fff5a3b T67890 M0x7fff5a4c T12345
```

### Giải thích chu trình:
```
T12345 → M0x7fff5a3b → T67890 → M0x7fff5a4c → T12345
   │          │           │          │          │
   │          │           │          │          └── Quay về T12345!
   │          │           │          └── M_B thuộc về T12345
   │          │           └── T67890 đang chờ M_B
   │          └── M_A thuộc về T67890
   └── T12345 đang chờ M_A
   
=> DEADLOCK: T12345 chờ T67890, T67890 chờ T12345
```

---

## 🎯 SLIDE 12: VÍ DỤ DEMO - DEADLOCK KINH ĐIỂN

**Nói:**
> "Hãy xem ví dụ deadlock kinh điển và cách libdd phát hiện nó."

### Code demo (demo_deadlock.c):
```c
#include <pthread.h>
#include <stdio.h>

static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

// Thread 1: lock A trước, rồi lock B
static void* t1(void* arg) {
    pthread_mutex_lock(&A);     // 1. T1 chiếm A
    msleep(100);                // 2. Chờ một chút
    pthread_mutex_lock(&B);     // 3. T1 muốn B → DEADLOCK!
    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return NULL;
}

// Thread 2: lock B trước, rồi lock A
static void* t2(void* arg) {
    pthread_mutex_lock(&B);     // 1. T2 chiếm B
    msleep(100);                // 2. Chờ một chút
    pthread_mutex_lock(&A);     // 3. T2 muốn A → DEADLOCK!
    pthread_mutex_unlock(&A);
    pthread_mutex_unlock(&B);
    return NULL;
}

int main(void) {
    pthread_t x, y;
    pthread_create(&x, NULL, t1, NULL);
    pthread_create(&y, NULL, t2, NULL);
    pthread_join(x, NULL);      // Sẽ treo vĩnh viễn!
    pthread_join(y, NULL);
    return 0;
}
```

---

## 🎯 SLIDE 13: MÔ PHỎNG TỪNG BƯỚC

**Nói:**
> "Hãy theo dõi đồ thị WFG thay đổi như thế nào."

### Timeline:
```
t=0ms:   T1 bắt đầu                    T2 bắt đầu
         │                              │
t=1ms:   T1: lock(A) ──────────────────┤
         │                              │
         │  WFG: M_A → T1              │
         │                              │
t=2ms:   ├─────────────────────────────T2: lock(B)
         │                              │
         │  WFG: M_A → T1              │
         │       M_B → T2              │
         │                              │
t=100ms: T1: lock(B) ─────────────────┤
         │  B đang do T2 giữ!          │
         │                              │
         │  WFG: M_A → T1              │
         │       M_B → T2              │
         │       T1 → M_B   ← THÊM!    │
         │                              │
         │  Kiểm tra chu trình...      │
         │  Chưa có!                   │
         │                              │
t=101ms: ├─────────────────────────────T2: lock(A)
         │                              A đang do T1 giữ!
         │                              │
         │  WFG: M_A → T1              │
         │       M_B → T2              │
         │       T1 → M_B              │
         │       T2 → M_A   ← THÊM!    │
         │                              │
         │  ⚠️ KIỂM TRA CHU TRÌNH:     │
         │  T1 → M_B → T2 → M_A → T1   │
         │  ^^^^^^^^^^^^^^^^^^^^^^^^   │
         │       CÓ CHU TRÌNH!         │
         │                              │
         │  OUTPUT:                    │
         │  [libdd] DEADLOCK cycle:    │
         │  T1 M_B T2 M_A T1           │
```

---

## 🎯 SLIDE 14: TRỰC QUAN HÓA ĐỒ THỊ

**Nói:**
> "Đây là hình ảnh đồ thị WFG tại thời điểm deadlock."

### Đồ thị WFG khi deadlock:
```
         ┌──────────────────────────┐
         │                          │
         ▼                          │
       ┌───┐      "T1 chờ M_B"    ┌───┐
       │T1 │─────────────────────→│M_B│
       └───┘                      └───┘
         ▲                          │
         │                          │ "M_B thuộc T2"
         │                          ▼
         │                        ┌───┐
         │                        │T2 │
         │                        └───┘
         │                          │
         │ "M_A thuộc T1"          │ "T2 chờ M_A"
         │                          ▼
       ┌───┐                      ┌───┐
       │M_A│←─────────────────────│M_A│
       └───┘                      └───┘
         │                          
         └──────────────────────────┘

CHU TRÌNH: T1 → M_B → T2 → M_A → T1

Giải thích:
• T1 giữ M_A, muốn M_B
• T2 giữ M_B, muốn M_A  
• Cả hai chờ nhau → DEADLOCK!
```

---

## 🎯 SLIDE 15: CHẠY THỬ VỚI LIBDD

**Nói:**
> "Đây là cách chạy chương trình với libdd."

### Biên dịch:
```bash
# Biên dịch libdd thành shared library
gcc -shared -fPIC -o libdd.so src/runtime/libdd.c \
    src/graph/graph.c src/common/util.c \
    -I include -ldl -lpthread

# Biên dịch demo
gcc -o demo_deadlock src/demo/demo_deadlock.c \
    src/common/util.c -I include -lpthread
```

### Chạy:
```bash
# Chạy bình thường (sẽ treo vĩnh viễn)
./demo_deadlock
# ^C để tắt

# Chạy với libdd (phát hiện deadlock)
LD_PRELOAD=./libdd.so ./demo_deadlock

# Output:
# [libdd] DEADLOCK cycle: T139812345 M0x7fff5a3b T139867890 M0x7fff5a4c T139812345

# Chạy với verbose log
DD_LOG_LEVEL=2 LD_PRELOAD=./libdd.so ./demo_deadlock

# Output:
# [libdd] init ok (log=2)
# [libdd] fastpath lock M0x7fff5a3b by T139812345
# [libdd] acquired M0x7fff5a3b -> T139812345 (count=1)
# [libdd] fastpath lock M0x7fff5a4c by T139867890
# [libdd] acquired M0x7fff5a4c -> T139867890 (count=1)
# [libdd] wait T139812345 -> M0x7fff5a4c (owner=T139867890)
# [libdd] wait T139867890 -> M0x7fff5a3b (owner=T139812345)
# [libdd] DEADLOCK cycle: T139812345 M0x7fff5a4c T139867890 M0x7fff5a3b T139812345
```

---

## 🎯 SLIDE 16: ĐỘ PHỨC TẠP VÀ OVERHEAD

**Nói:**
> "Hãy phân tích performance overhead của libdd."

### Độ phức tạp mỗi thao tác:

| Thao tác | Độ phức tạp | Giải thích |
|----------|-------------|------------|
| lock() - best case | O(1) | Mutex free, không cần kiểm tra cycle |
| lock() - worst case | O(V + E) | Mutex bận, phải kiểm tra cycle bằng DFS |
| unlock() | O(1) | Chỉ gỡ cạnh và cập nhật bảng |
| trylock() | O(1) | Không chờ, không kiểm tra cycle |

Trong đó:
- V = số node (threads + mutexes)
- E = số cạnh (quan hệ chờ/sở hữu)

### Overhead thực tế:
```
┌─────────────────────────────────────────────────────────┐
│ Benchmark: 1 triệu lock/unlock operations              │
├─────────────────────────────────────────────────────────┤
│ Không có libdd:  ~50ms                                 │
│ Có libdd (log=0): ~200ms  (4x overhead)               │
│ Có libdd (log=2): ~500ms  (10x overhead - do I/O)     │
└─────────────────────────────────────────────────────────┘

⚠️ Overhead đáng kể → Chỉ dùng khi debug/testing!
```

### Tối ưu hóa đã áp dụng:
✅ Debounce log (200ms) - tránh spam  
✅ Fastpath khi mutex free - bỏ qua cycle check  
✅ Thread-local reentrancy guard - tránh lock contention  

---

## 🎯 SLIDE 17: SO SÁNH 3 PHƯƠNG PHÁP

**Nói:**
> "Tổng kết so sánh 3 phương pháp phát hiện deadlock."

### Bảng so sánh:

| Tiêu chí | WFG (Offline) | Matrix (Offline) | Runtime (libdd) |
|----------|---------------|------------------|-----------------|
| **Thời điểm** | Phân tích file | Phân tích file | Khi chạy |
| **Input** | Đồ thị chờ | Ma trận A, Need, Avail | Không cần |
| **Độ phức tạp** | O(V + E) | O(N² × M) | O(V + E) mỗi lock |
| **Loại tài nguyên** | Single-instance | Multi-instance | Single-instance |
| **Overhead** | Không | Không | **Cao** |
| **Ưu điểm** | Nhanh, đơn giản | Hỗ trợ multi-instance | Không cần sửa code |
| **Nhược điểm** | Cần đồ thị sẵn | Cần biết Max need | Chậm runtime |

### Khi nào dùng phương pháp nào?

| Tình huống | Phương pháp |
|------------|-------------|
| Phân tích thiết kế hệ thống | WFG hoặc Matrix |
| Debug ứng dụng có sẵn | **Runtime (libdd)** |
| Hệ thống production | Không nên dùng libdd (overhead cao) |
| Nhiều instance mỗi tài nguyên | Matrix |

---

## 🎯 SLIDE 18: TỔNG KẾT

**Nói:**
> "Tóm tắt lại những điểm chính về libdd - Runtime Deadlock Detection."

### Cơ chế hoạt động:
```
1️⃣ HOOK: Thay thế pthread_mutex_lock/unlock bằng LD_PRELOAD

2️⃣ THEO DÕI: Xây dựng Wait-For Graph động
   • T → M: Thread đang chờ mutex
   • M → T: Mutex thuộc về thread

3️⃣ PHÁT HIỆN: Kiểm tra chu trình mỗi khi thêm cạnh chờ

4️⃣ BÁO CÁO: In chu trình deadlock ra stderr
```

### Ưu điểm:
✅ Không cần sửa source code ứng dụng  
✅ Phát hiện deadlock theo thời gian thực  
✅ Chỉ ra chính xác threads và mutexes liên quan  
✅ Dễ tích hợp (chỉ cần LD_PRELOAD)  

### Nhược điểm:
❌ Overhead cao (không dùng cho production)  
❌ Chỉ hỗ trợ pthread mutex (không hỗ trợ semaphore, etc.)  
❌ Chỉ chạy trên Linux (cần dlsym, LD_PRELOAD)  
❌ Chỉ phát hiện, không tự giải quyết deadlock  

---

## 🤔 CÂU HỎI THƯỜNG GẶP

**Q1: Tại sao dùng LD_PRELOAD thay vì sửa source code?**
> A: Để có thể debug bất kỳ ứng dụng nào mà không cần access source code. Đặc biệt hữu ích với third-party libraries.

**Q2: libdd có thể ngăn chặn deadlock không?**
> A: Không, libdd chỉ PHÁT HIỆN. Để ngăn chặn, cần sử dụng các kỹ thuật như lock ordering hoặc timeout.

**Q3: Tại sao cần debounce 200ms?**
> A: Khi deadlock xảy ra, các thread có thể retry lock nhiều lần/giây. Debounce tránh spam hàng ngàn dòng log giống nhau.

**Q4: libdd có thread-safe không?**
> A: Có, sử dụng internal mutex (g_mu) để bảo vệ global state. Tuy nhiên điều này cũng tạo thêm overhead.

**Q5: Có thể dùng với C++ std::mutex không?**
> A: Trên hầu hết các implementation, std::mutex internally gọi pthread_mutex, nên libdd vẫn hoạt động.

---

## 📚 TÀI LIỆU THAM KHẢO

1. **dlsym manual**: `man dlsym` - Dynamic linking functions
2. **LD_PRELOAD trick**: "How to Hook Library Functions" 
3. **pthread_mutex**: POSIX Threads Programming Guide
4. **Deadlock Detection Algorithms**: Operating System Concepts, Chapter 8

---

**🎯 CẢM ƠN MỌI NGƯỜI ĐÃ LẮNG NGHE!**

*Có câu hỏi nào không ạ?*
