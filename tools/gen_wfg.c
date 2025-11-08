#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int u,v; } Edge;

static unsigned long long s = 88172645463393265ULL;
static unsigned rndu(void){ s ^= s<<7; s ^= s>>9; return (unsigned)(s & 0xffffffffu); }
static void sseed(unsigned long long x){ if(x==0) x=1; s=x; }
static int rndint(int lo,int hi){ unsigned r = rndu(); int span = hi-lo+1; return lo + (int)(r% (unsigned)span); }

int main(int argc,char**argv){
    if(argc<3){
        fprintf(stderr,"Usage: %s N E [--seed S] [--mode random|cycle|acyclic] [--cycle-len K]\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);
    int E = atoi(argv[2]);
    unsigned long long seed = 1;
    const char* mode = "random";
    int K = 3;

    for(int i=3;i<argc;i++){
        if(!strcmp(argv[i],"--seed") && i+1<argc) seed = strtoull(argv[++i],NULL,10);
        else if(!strcmp(argv[i],"--mode") && i+1<argc) mode = argv[++i];
        else if(!strcmp(argv[i],"--cycle-len") && i+1<argc) K = atoi(argv[++i]);
    }
    if(N<=0 || E<0){ fprintf(stderr,"Bad N/E\n"); return 1; }
    sseed(seed);

    Edge *edges = (Edge*)malloc((size_t)E*sizeof(*edges));
    if(!edges){ perror("malloc"); return 1; }

    int used=0;

    if(!strcmp(mode,"cycle")){
        if(K<2) K=2;
        if(K>N) K=N;
        /* tạo một chu trình K-đỉnh: 0->1->...->K-1->0 */
        for(int i=0;i<K;i++){
            edges[used].u = i;
            edges[used].v = (i+1==K)?0:(i+1);
            used++;
            if(used==E) break;
        }
        /* bổ sung ngẫu nhiên các cạnh còn lại (có thể trùng, chấp nhận) */
        while(used<E){
            int u = rndint(0,N-1);
            int v = rndint(0,N-1);
            edges[used].u = u; edges[used].v = v;
            used++;
        }
    }else if(!strcmp(mode,"acyclic")){
        /* sinh DAG bằng cách chỉ cho u<v */
        while(used<E){
            int u = rndint(0,N-2);
            int v = rndint(u+1,N-1);
            edges[used].u = u; edges[used].v = v;
            used++;
        }
    }else{
        /* random tự do */
        while(used<E){
            int u = rndint(0,N-1);
            int v = rndint(0,N-1);
            edges[used].u = u; edges[used].v = v;
            used++;
        }
    }

    /* in ra format WFG */
    printf("%d %d\n", N, E);
    for(int i=0;i<E;i++){
        printf("%d %d\n", edges[i].u, edges[i].v);
    }
    free(edges);
    return 0;
}
