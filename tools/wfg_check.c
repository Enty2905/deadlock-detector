#include "graph.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char**argv){
    if(argc==2){
        if(!freopen(argv[1],"r",stdin)){
            perror("freopen"); return 1;
        }
    }
    int P,E; if(scanf("%d %d",&P,&E)!=2) return 1;

    graph_t* g=graph_create();
    for(int i=0;i<P;i++){
        char lb[32]; snprintf(lb,sizeof(lb),"P%d",i);
        graph_add_node(g,lb);
    }
    for(int i=0;i<E;i++){
        int u,v; if(scanf("%d %d",&u,&v)!=2) return 1;
        graph_add_edge(g,u,v);
    }

    int* cyc=NULL; size_t L=0;
    if(graph_find_cycle(g,&cyc,&L)){
        printf("DEADLOCK cycle: ");
        for(size_t i=0;i<L;i++)
            printf("%s%s", graph_node_label(g,cyc[i]), (i+1<L)?" ":"");
        printf("\n");
        free(cyc);
    } else {
        puts("NO DEADLOCK");
    }

    graph_free(g);
    return 0;
}
