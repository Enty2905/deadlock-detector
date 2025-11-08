#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "graph.h"

typedef struct { int* a; size_t n, cap; } veci;

static void veci_init(veci* v){ v->a=NULL; v->n=0; v->cap=0; }
static void veci_free(veci* v){ free(v->a); v->a=NULL; v->n=v->cap=0; }
static void veci_push(veci* v, int x){
    if(v->n==v->cap){
        size_t ncap = v->cap? v->cap*2 : 4;
        v->a = (int*)xrealloc(v->a, ncap*sizeof(int));
        v->cap = ncap;
    }
    v->a[v->n++] = x;
}
static void veci_remove_one(veci* v, int x){
    for(size_t i=0;i<v->n;i++){
        if(v->a[i]==x){
            memmove(&v->a[i], &v->a[i+1], (v->n-i-1)*sizeof(int));
            v->n--;
            return;
        }
    }
}

struct graph{
    int    n;        /* số node hiện tại */
    veci*  adj;      /* adj[u] là danh sách kề của u */
    char** label;    /* nhãn (có thể NULL cho đồ thị tĩnh) */
};

/* ---------- tiện ích nội bộ ---------- */
static graph_t* graph_alloc(int n){
    graph_t* g = (graph_t*)xmalloc(sizeof(*g));
    g->n = n;
    g->adj   = (veci*)xcalloc((size_t)n, sizeof(veci));
    g->label = (char**)xcalloc((size_t)n, sizeof(char*));
    for(int i=0;i<n;i++) veci_init(&g->adj[i]);
    return g;
}
static void graph_reserve_nodes(graph_t* g, int need){
    if(need <= g->n) return;
    int old = g->n;
    g->adj   = (veci*)xrealloc(g->adj,   (size_t)need*sizeof(veci));
    g->label = (char**)xrealloc(g->label,(size_t)need*sizeof(char*));
    for(int i=old;i<need;i++){ veci_init(&g->adj[i]); g->label[i]=NULL; }
    g->n = need;
}
static char* xstrdup(const char* s){
    size_t L = strlen(s)+1;
    char* p = (char*)xmalloc(L);
    memcpy(p,s,L);
    return p;
}

/* ---------- API tĩnh ---------- */
graph_t* graph_new(int n){ return graph_alloc(n); }

void graph_free(graph_t* g){
    if(!g) return;
    for(int i=0;i<g->n;i++){
        veci_free(&g->adj[i]);
        free(g->label[i]);
    }
    free(g->adj);
    free(g->label);
    free(g);
}

void graph_add_edge(graph_t* g, int u, int v){
    if(!g) return;
    if(u<0 || v<0 || u>=g->n || v>=g->n) return;
    /* tránh thêm trùng (đơn giản) */
    for(size_t i=0;i<g->adj[u].n;i++) if(g->adj[u].a[i]==v) return;
    veci_push(&g->adj[u], v);
}

/* DFS tìm 1 chu trình, trả về true nếu có; cycle (v..u..v) */
static bool dfs_backedge(const graph_t* g, int u, int* color, int* parent,
                         int* out_u, int* out_v){
    color[u]=1; /* gray */
    for(size_t i=0;i<g->adj[u].n;i++){
        int v=g->adj[u].a[i];
        if(color[v]==0){
            parent[v]=u;
            if(dfs_backedge(g,v,color,parent,out_u,out_v)) return true;
        }else if(color[v]==1){
            *out_u=u; *out_v=v; /* back-edge u->v */
            return true;
        }
    }
    color[u]=2; /* black */
    return false;
}

bool graph_find_cycle(const graph_t* g, int** cycle_ids, size_t* cycle_len){
    *cycle_ids=NULL; *cycle_len=0;
    if(!g || g->n==0) return false;

    int* color  = (int*)xcalloc((size_t)g->n, sizeof(int));
    int* parent = (int*)xcalloc((size_t)g->n, sizeof(int));
    for(int i=0;i<g->n;i++) parent[i]=-1;

    int u=-1,v=-1;
    bool found=false;
    for(int s=0; s<g->n && !found; s++){
        if(color[s]==0) found = dfs_backedge(g,s,color,parent,&u,&v);
    }

    if(found){
        /* reconstruct path v .. u plus v to close */
        /* trước hết đếm độ dài */
        int cur=u; size_t k=1; /* sẽ push u.. cho đến v */
        while(cur!=v && cur!=-1){ cur=parent[cur]; k++; }
        if(cur==-1){ /* phòng thủ: không tìm ngược được, trả về false */
            free(color); free(parent);
            return false;
        }
        int* cyc = (int*)xmalloc((k+1)*sizeof(int));
        /* điền ngược */
        size_t idx=k-1; cyc[idx]=v;
        cur=u;
        while(cur!=v){ cyc[--idx]=cur; cur=parent[cur]; }
        cyc[k]=v; /* đóng vòng */

        *cycle_ids = cyc;
        *cycle_len = k+1;
        free(color); free(parent);
        return true;
    }
    free(color); free(parent);
    return false;
}

/* ---------- API động cho runtime ---------- */
graph_t* graph_create(void){ return graph_alloc(0); }

int graph_add_node(graph_t* g, const char* label){
    if(!g) return -1;
    int id = g->n;
    graph_reserve_nodes(g, id+1);
    if(label){
        free(g->label[id]);
        g->label[id] = xstrdup(label);
    }
    return id;
}

int graph_get_or_add_node(graph_t* g, const char* label){
    if(!g || !label) return -1;
    for(int i=0;i<g->n;i++){
        if(g->label[i] && strcmp(g->label[i], label)==0) return i;
    }
    return graph_add_node(g, label);
}

const char* graph_node_label(const graph_t* g, int id){
    static __thread char buf[64];
    if(!g || id<0 || id>=g->n) return "(out_of_range)";
    if(g->label[id]) return g->label[id];
    snprintf(buf,sizeof(buf),"P%d", id);
    return buf;
}

void graph_remove_edge(graph_t* g, int u, int v){
    if(!g) return;
    if(u<0 || u>=g->n) return;
    veci_remove_one(&g->adj[u], v);
}
