#include "graph.h"

#include <string.h>

#include "util.h"

typedef struct {
    int* data;
    size_t size, cap;
} ivec_t;
static void ivec_init(ivec_t* v) {
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}
static void ivec_push(ivec_t* v, int x) {
    if (v->size == v->cap) {
        v->cap = v->cap ? v->cap * 2 : 4;
        v->data = xrealloc(v->data, v->cap * sizeof(int));
    }
    v->data[v->size++] = x;
}
static int ivec_remove_first(ivec_t* v, int x) {
    for (size_t i = 0; i < v->size; i++) {
        if (v->data[i] == x) {
            memmove(&v->data[i], &v->data[i + 1], (v->size - i - 1) * sizeof(int));
            v->size--;
            return 1;
        }
    }
    return 0;
}

typedef struct {
    char* label;
    ivec_t adj;
} node_t;
struct graph {
    node_t* nodes;
    size_t n, cap;
};

static int graph_add_node_raw(graph_t* g) {
    if (g->n == g->cap) {
        g->cap = g->cap ? g->cap * 2 : 8;
        g->nodes = xrealloc(g->nodes, g->cap * sizeof(node_t));
    }
    node_t* nd = &g->nodes[g->n];
    nd->label = NULL;
    ivec_init(&nd->adj);
    return (int)g->n++;
}
graph_t* graph_create(void) { return xcalloc(1, sizeof(graph_t)); }
void graph_free(graph_t* g) {
    if (!g)
        return;
    for (size_t i = 0; i < g->n; i++) {
        free(g->nodes[i].label);
        free(g->nodes[i].adj.data);
    }
    free(g->nodes);
    free(g);
}
const char* graph_node_label(const graph_t* g, int id) {
    if (!g || id < 0 || (size_t)id >= g->n)
        return NULL;
    return g->nodes[id].label ? g->nodes[id].label : "";
}
size_t graph_num_nodes(const graph_t* g) { return g ? g->n : 0; }

int graph_add_node(graph_t* g, const char* label) {
    int id = graph_add_node_raw(g);
    if (label) {
        size_t L = strlen(label) + 1;
        g->nodes[id].label = xmalloc(L);
        memcpy(g->nodes[id].label, label, L);
    }
    return id;
}
int graph_get_or_add_node(graph_t* g, const char* label) {
    if (!label)
        return graph_add_node(g, NULL);
    for (size_t i = 0; i < g->n; i++) {
        if (g->nodes[i].label && strcmp(g->nodes[i].label, label) == 0)
            return (int)i;
    }
    return graph_add_node(g, label);
}
void graph_add_edge(graph_t* g, int u, int v) {
    if (!g || u < 0 || v < 0 || (size_t)u >= g->n || (size_t)v >= g->n)
        return;
    for (size_t i = 0; i < g->nodes[u].adj.size; i++)
        if (g->nodes[u].adj.data[i] == v)
            return;
    ivec_push(&g->nodes[u].adj, v);
}
void graph_remove_edge(graph_t* g, int u, int v) {
    if (!g || u < 0 || v < 0 || (size_t)u >= g->n || (size_t)v >= g->n)
        return;
    ivec_remove_first(&g->nodes[u].adj, v);
}

/* DFS: 0 white, 1 gray, 2 black. Tìm back-edge u->v */
typedef struct {
    int* color;
    int* parent;
    int u, v;
} dfs_ctx_t;
static int dfs_visit(const graph_t* g, int s, dfs_ctx_t* c) {
    c->color[s] = 1;
    const ivec_t* adj = &g->nodes[s].adj;
    for (size_t k = 0; k < adj->size; k++) {
        int t = adj->data[k];
        if (c->color[t] == 0) {
            c->parent[t] = s;
            if (dfs_visit(g, t, c))
                return 1;
        } else if (c->color[t] == 1) {
            c->u = s;
            c->v = t;
            return 1;
        }
    }
    c->color[s] = 2;
    return 0;
}
bool graph_find_cycle(const graph_t* g, int** out, size_t* out_len) {
    if (out)
        *out = NULL;
    if (out_len)
        *out_len = 0;
    if (!g || g->n == 0)
        return false;
    dfs_ctx_t c = {.color = xcalloc(g->n, sizeof(int)),
                   .parent = xcalloc(g->n, sizeof(int)),
                   .u = -1,
                   .v = -1};
    for (size_t i = 0; i < g->n; i++) c.parent[i] = -1;
    int found = 0;
    for (size_t i = 0; i < g->n && !found; i++)
        if (c.color[i] == 0)
            found = dfs_visit(g, (int)i, &c);
    if (!found) {
        free(c.color);
        free(c.parent);
        return false;
    }

    /* Dựng chu trình theo thứ tự xuôi: v -> ... -> u -> v */
    int u = c.u, v = c.v;
    int cap = 8, L = 0;
    int* cyc = xmalloc(cap * sizeof(int));
    int tcap = 8, tL = 0;
    int* tmp = xmalloc(tcap * sizeof(int));
    tmp[tL++] = u;
    for (int x = c.parent[u]; x != -1 && x != v; x = c.parent[x]) {
        if (tL == tcap) {
            tcap *= 2;
            tmp = xrealloc(tmp, tcap * sizeof(int));
        }
        tmp[tL++] = x;
    }
    if (L == cap) {
        cap *= 2;
        cyc = xrealloc(cyc, cap * sizeof(int));
    }
    cyc[L++] = v;
    for (int i = tL - 1; i >= 0; i--) {
        if (L == cap) {
            cap *= 2;
            cyc = xrealloc(cyc, cap * sizeof(int));
        }
        cyc[L++] = tmp[i];
    }
    if (L == cap) {
        cap *= 2;
        cyc = xrealloc(cyc, cap * sizeof(int));
    }
    cyc[L++] = v;
    free(tmp);
    if (out)
        *out = cyc;
    else
        free(cyc);
    if (out_len)
        *out_len = (size_t)L;
    free(c.color);
    free(c.parent);
    return true;
}
