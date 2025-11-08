#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "util.h"


int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "Usage: %s <input>\n", argv[0]);
        return 2;
    }
    FILE* f = fopen(argv[1], "r");
    if(!f){ perror("fopen"); return 2; }

    int N,M;
    if(fscanf(f, "%d %d", &N, &M) != 2){
        fprintf(stderr, "Bad header\n");
        fclose(f); return 2;
    }

    int **A = (int**)xcalloc(N, sizeof(int*));
    int **Need = (int**)xcalloc(N, sizeof(int*));
    int  *Avail = (int*)xcalloc(M, sizeof(int));

    for(int i=0;i<N;i++){
        A[i] = (int*)xcalloc(M, sizeof(int));
        for(int j=0;j<M;j++){
            if(fscanf(f, "%d", &A[i][j]) != 1){ fprintf(stderr, "Bad A\n"); return 2; }
        }
    }
    for(int i=0;i<N;i++){
        Need[i] = (int*)xcalloc(M, sizeof(int));
        for(int j=0;j<M;j++){
            if(fscanf(f, "%d", &Need[i][j]) != 1){ fprintf(stderr, "Bad Need\n"); return 2; }
        }
    }
    for(int j=0;j<M;j++){
        if(fscanf(f, "%d", &Avail[j]) != 1){ fprintf(stderr, "Bad Available\n"); return 2; }
    }
    fclose(f);

    int *Work = (int*)xcalloc(M, sizeof(int));
    bool *Finish = (bool*)xcalloc(N, sizeof(bool));
    for(int j=0;j<M;j++) Work[j] = Avail[j];

    bool progress = true;
    while(progress){
        progress = false;
        for(int i=0;i<N;i++){
            if(Finish[i]) continue;
            bool ok = true;
            for(int j=0;j<M;j++){
                if(Need[i][j] > Work[j]){ ok=false; break; }
            }
            if(ok){
                for(int j=0;j<M;j++) Work[j] += A[i][j];
                Finish[i] = true;
                progress = true;
            }
        }
    }

    int *dead = (int*)xcalloc(N, sizeof(int));
    int K = 0;
    for(int i=0;i<N;i++) if(!Finish[i]) dead[K++] = i;

    if(K == 0){
        puts("NO DEADLOCK");
    }else{
        printf("DEADLOCK on %d process(es):", K);
        for(int t=0;t<K;t++) printf(" P%d", dead[t]);
        putchar('\n');
    }

    for(int i=0;i<N;i++){ free(A[i]); free(Need[i]); }
    free(A); free(Need); free(Avail); free(Work); free(Finish); free(dead);
    return 0;
}
