#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"

/* ===== logging level (mặc định = 1) ===== */
static int g_log = 1;

/* ===== giữ địa chỉ thật của pthread APIs ===== */
static int (*real_mutex_lock)(pthread_mutex_t*) = NULL;
static int (*real_mutex_trylock)(pthread_mutex_t*) = NULL;
static int (*real_mutex_unlock)(pthread_mutex_t*) = NULL;

/* ===== đồ thị chờ giữa THREADS ===== */
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;  /* bảo vệ state toàn cục */
static graph_t* g_wfg = NULL;

/* ===== bảng (mutex -> owner) đơn giản ===== */
typedef struct {
    pthread_mutex_t* m;
    unsigned long    owner;  /* thread id cast sang số */
    int              count;  /* phòng trường hợp mutex recursive */
} mrec_t;

static mrec_t* g_tab = NULL;
static size_t  g_sz  = 0, g_cap = 0;

static inline unsigned long tid_self(void){
    return (unsigned long)pthread_self();
}

/* ---- bảng mutex-owner ---- */
static void tab_reserve(size_t need){
    if (g_cap >= need) return;
    size_t nc = g_cap ? g_cap*2 : 16;
    if (nc < need) nc = need;
    g_tab = (mrec_t*)realloc(g_tab, nc*sizeof(mrec_t));
    if (!g_tab){ perror("realloc g_tab"); abort(); }
    memset(g_tab + g_cap, 0, (nc - g_cap)*sizeof(mrec_t));
    g_cap = nc;
}
static mrec_t* tab_find(pthread_mutex_t* m){
    for (size_t i=0;i<g_sz;i++) if (g_tab[i].m == m) return &g_tab[i];
    return NULL;
}
static mrec_t* tab_get_or_add(pthread_mutex_t* m){
    mrec_t* r = tab_find(m);
    if (r) return r;
    tab_reserve(g_sz+1);
    g_tab[g_sz].m = m; g_tab[g_sz].owner = 0; g_tab[g_sz].count = 0;
    return &g_tab[g_sz++];
}

/* ---- graph helpers ---- */
static int node_of_thread(unsigned long t){
    char lb[64];
    snprintf(lb, sizeof(lb), "T%lu", t);
    return graph_get_or_add_node(g_wfg, lb);
}
static void log_cycle_if_any(void){
    int* cyc=NULL; size_t L=0;
    if (graph_find_cycle(g_wfg, &cyc, &L)){
        if (g_log){
            fprintf(stderr, "[libdd] DEADLOCK cycle: ");
            for (size_t i=0;i<L;i++){
                fprintf(stderr, "%s%s", graph_node_label(g_wfg, cyc[i]), (i+1<L)?" ":"");
            }
            fputc('\n', stderr);
        }
        free(cyc);
    }
}

/* ---- tránh -Wpedantic khi lấy symbol function pointer ---- */
#define LOAD_SYM(dst, name) do {                         \
    void* __p = dlsym(RTLD_NEXT, (name));                \
    if (!__p){                                           \
        fprintf(stderr, "[libdd] dlsym %s failed: %s\n", \
                (name), dlerror());                      \
        abort();                                         \
    }                                                    \
    *(void**)&(dst) = __p;                               \
} while(0)

/* ---- wrappers: khóa/unlock nội bộ bằng HÀM THẬT ---- */
static inline void mu_lock(void){
    if (!real_mutex_lock){ fprintf(stderr,"[libdd] real_mutex_lock NULL\n"); abort(); }
    (void)real_mutex_lock(&g_mu);
}
static inline void mu_unlock(void){
    if (!real_mutex_unlock){ fprintf(stderr,"[libdd] real_mutex_unlock NULL\n"); abort(); }
    (void)real_mutex_unlock(&g_mu);
}

/* ===== constructor/destructor ===== */
__attribute__((constructor))
static void dd_init(void){
    LOAD_SYM(real_mutex_lock,    "pthread_mutex_lock");
    LOAD_SYM(real_mutex_trylock, "pthread_mutex_trylock");
    LOAD_SYM(real_mutex_unlock,  "pthread_mutex_unlock");

    const char* lv = getenv("DD_LOG_LEVEL");
    if (lv && *lv) g_log = atoi(lv);

    g_wfg = graph_create();
    if (!g_wfg){
        fprintf(stderr, "[libdd] graph_create failed\n");
        abort();
    }
}

__attribute__((destructor))
static void dd_fini(void){
    free(g_tab); g_tab=NULL; g_sz=g_cap=0;
    graph_free(g_wfg); g_wfg=NULL;
}

/* ===== hooks ===== */

int pthread_mutex_trylock(pthread_mutex_t* m){
    /* trylock KHÔNG chờ -> không thêm cạnh chờ để tránh false positive */
    return real_mutex_trylock(m);
}

int pthread_mutex_lock(pthread_mutex_t* m){
    unsigned long me = tid_self();
    int rc_wait_edge_added = 0;
    int owner_node = -1, me_node = -1;

    /* Trước khi gọi lock thật: nếu mutex đang do owner != me giữ -> me CHỜ owner */
    mu_lock();
    mrec_t* r = tab_get_or_add(m);
    unsigned long owner = r->owner;

    if (owner != 0 && owner != me){
        me_node    = node_of_thread(me);
        owner_node = node_of_thread(owner);
        graph_add_edge(g_wfg, me_node, owner_node);   /* me -> owner (chờ) */
        rc_wait_edge_added = 1;
        log_cycle_if_any();
    }
    mu_unlock();

    /* Gọi lock thật (có thể block) */
    int rc = real_mutex_lock(m);
    if (rc != 0) return rc;

    /* Đã khoá thành công:
       - gỡ cạnh chờ (nếu có)
       - cập nhật owner = me (+count) */
    mu_lock();
    if (rc_wait_edge_added && me_node>=0 && owner_node>=0){
        graph_remove_edge(g_wfg, me_node, owner_node);
    }
    r = tab_get_or_add(m);
    if (r->owner == me){
        r->count += 1;
    } else {
        r->owner = me; r->count = 1;
    }
    mu_unlock();
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* m){
    int rc = real_mutex_unlock(m);
    if (rc != 0) return rc;

    mu_lock();
    mrec_t* r = tab_get_or_add(m);
    unsigned long me = tid_self();
    if (r->owner == me){
        if (r->count > 1) r->count -= 1;
        else { r->count = 0; r->owner = 0; }
    }
    mu_unlock();
    return 0;
}
