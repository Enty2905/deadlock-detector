#include "graph.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct { int u,v; } Edge;

static void write_dot(const char* path, int N, int E, const Edge* edges,
                      const int* cyc, size_t clen){
    FILE* f = fopen(path,"w");
    if(!f){ perror("fopen dot"); exit(EX_INTERNAL); }

    bool *in_node = (bool*)xcalloc((size_t)N, sizeof(bool));
    bool *in_edge = (bool*)xcalloc((size_t)E, sizeof(bool));

    if(cyc && clen>1){
        for(size_t i=0;i<clen;i++){
            int a = cyc[i];
            int b = cyc[(i+1)%clen]; /* chu trình đóng (node cuối nối về đầu) */
            if(a>=0 && a<N) in_node[a]=true;
            if(b>=0 && b<N) in_node[b]=true;
            for(int k=0;k<E;k++){
                if(edges[k].u==a && edges[k].v==b){ in_edge[k]=true; break; }
            }
        }
    }

    fprintf(f,"digraph WFG {\n  rankdir=LR;\n  node [shape=circle, fontsize=11];\n");
    for(int i=0;i<N;i++){
        if(in_node[i]) fprintf(f,"  P%d [color=red, fontcolor=red];\n", i);
        else           fprintf(f,"  P%d;\n", i);
    }
    for(int k=0;k<E;k++){
        const char* attr = in_edge[k] ? "[color=red, penwidth=2.0]" : "";
        fprintf(f,"  P%d -> P%d %s;\n", edges[k].u, edges[k].v, attr);
    }
    fprintf(f,"}\n");
    fclose(f);
    free(in_node); free(in_edge);
}

int main(int argc,char**argv){
    if(argc<2 || argc>3){
        fprintf(stderr,"Usage: %s input.in [output.dot]\n", argv[0]);
        return EX_BAD_INPUT;
    }
    const char* in  = argv[1];
    const char* out = (argc==3)? argv[2] : "out.dot";

    if(!freopen(in,"r",stdin)){ perror("freopen"); return EX_BAD_INPUT; }

    int N,E;
    if(scanf("%d %d",&N,&E)!=2 || N<=0 || E<0){ fprintf(stderr,"Bad N E\n"); return EX_BAD_INPUT; }
    Edge *edges = (Edge*)xmalloc((size_t)E*sizeof(*edges));

    graph_t *g = graph_new(N);
    for(int i=0;i<E;i++){
        int u,v;
        if(scanf("%d %d",&u,&v)!=2){ fprintf(stderr,"Bad edge\n"); free(edges); graph_free(g); return EX_BAD_INPUT; }
        edges[i].u = u; edges[i].v = v;
        graph_add_edge(g,u,v);
    }

    int* cyc_ids = NULL; size_t clen = 0;
    (void)graph_find_cycle(g, &cyc_ids, &clen); /* có/không đều xuất DOT */

    write_dot(out, N, E, edges, cyc_ids, clen);

    if(cyc_ids){
        printf("DEADLOCK cycle: ");
        for(size_t i=0;i<clen;i++){
            printf("P%d%s", cyc_ids[i], (i+1<clen)?" ":"");
        }
        printf("\n");
        free(cyc_ids);
    }else{
        printf("NO DEADLOCK\n");
    }

    free(edges);
    graph_free(g);
    return EX_OK;
}
