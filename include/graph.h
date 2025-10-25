#ifndef GRAPH_H
#define GRAPH_H
#include <stdbool.h>
#include <stddef.h>
typedef struct graph graph_t;
graph_t* graph_create(void);
void graph_free(graph_t* g);
int graph_add_node(graph_t* g, const char* label);
int graph_get_or_add_node(graph_t* g, const char* label);
const char* graph_node_label(const graph_t* g, int id);
size_t graph_num_nodes(const graph_t* g);
void graph_add_edge(graph_t* g, int u, int v);
void graph_remove_edge(graph_t* g, int u, int v);
bool graph_find_cycle(const graph_t* g, int** cycle_ids, size_t* cycle_len);
#endif
