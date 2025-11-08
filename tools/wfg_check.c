#include "graph.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
    if(argc==2){
        if(!freopen(argv[1], "r", stdin)){ perror("freopen"); return EX_BAD_INPUT; }
    }
    int N,E;
    if(scanf("%d %d",&N,&E)!=2 || N<=0 || E<0){ fprintf(stderr,"Bad N E\n"); return EX_BAD_INPUT; }
    graph_t *g = graph_new(N);
    for(int i=0;i<E;i++){
        int u,v;
        if(scanf("%d %d",&u,&v)!=2){ fprintf(stderr,"Bad edge\n"); graph_free(g); return EX_BAD_INPUT; }
        graph_add_edge(g,u,v);
    }
    int *cyc=NULL; size_t clen=0;
    if(graph_find_cycle(g,&cyc,&clen)){
        printf("DEADLOCK cycle: ");
        for(size_t i=0;i<clen;i++){
            printf("P%d%s", cyc[i], (i+1<clen)?" ":"");
        }
        printf("\n");
        free(cyc);
        graph_free(g);
        return EX_DEADLOCK;
    }else{
        printf("NO DEADLOCK\n");
        graph_free(g);
        return EX_OK;
    }
}
