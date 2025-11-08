#ifndef GRAPH_H
#define GRAPH_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct graph graph_t;

/* === API tĩnh (phục vụ tools đọc WFG với N biết trước) === */
graph_t* graph_new(int n);           /* tạo đồ thị với n nút [0..n-1] */
void     graph_free(graph_t* g);
void     graph_add_edge(graph_t* g, int u, int v);
bool     graph_find_cycle(const graph_t* g, int** cycle_ids, size_t* cycle_len);

/* === API động (runtime libdd cần) === */
graph_t*     graph_create(void);                             /* bắt đầu rỗng */
int          graph_add_node(graph_t* g, const char* label);  /* thêm node, trả id */
int          graph_get_or_add_node(graph_t* g, const char* label);
const char*  graph_node_label(const graph_t* g, int id);
void         graph_remove_edge(graph_t* g, int u, int v);    /* xóa 1 cạnh nếu có */

#ifdef __cplusplus
}
#endif
#endif
