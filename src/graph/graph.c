#include "graph.h"
#include "util.h"
struct graph { int dummy; };
graph_t* graph_create(void){ return (graph_t*)xcalloc(1,sizeof(graph_t)); }
void     graph_free(graph_t* g){ if(g) free(g); }
int      graph_add_node(graph_t* g, const char* label){ (void)g;(void)label; return 0; }
int      graph_get_or_add_node(graph_t* g, const char* label){ (void)g;(void)label; return 0; }
const char* graph_node_label(const graph_t* g, int id){ (void)g;(void)id; return ""; }
size_t   graph_num_nodes(const graph_t* g){ (void)g; return 0; }
void     graph_add_edge(graph_t* g, int u, int v){ (void)g;(void)u;(void)v; }
void     graph_remove_edge(graph_t* g, int u, int v){ (void)g;(void)u;(void)v; }
bool     graph_find_cycle(const graph_t* g, int** ids, size_t* len){ (void)g; if(ids)*ids=NULL; if(len)*len=0; return false; }
