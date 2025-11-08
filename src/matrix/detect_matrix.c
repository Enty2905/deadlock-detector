#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static void* xmalloc(size_t n){
    void* p = malloc(n);
    if(!p){ perror("malloc"); exit(1); }
    return p;
}

int main(int argc, char** argv){
    if(argc==2){
        if(!freopen(argv[1], "r", stdin)){ perror("freopen"); return 1; }
    }

    int N=0, M=0;
    if (scanf("%d %d", &N, &M) != 2 || N <= 0 || M <= 0){
        fprintf(stderr, "Bad N M\n");
        return 1;
    }

    // Khởi tạo con trỏ về NULL để cleanup an toàn
    int **A = NULL, **R = NULL;
    int *Avail = NULL, *Work = NULL;
    bool *Finish = NULL;

    // Allocation
    A = xmalloc((size_t)N * sizeof *A);
    for(int i=0;i<N;i++){
        A[i] = xmalloc((size_t)M * sizeof **A);
        for(int j=0;j<M;j++){
            if (scanf("%d", &A[i][j]) != 1){
                fprintf(stderr, "Bad Allocation at (%d,%d)\n", i, j);
                goto cleanup;
            }
        }
    }

    // Need / Request
    R = xmalloc((size_t)N * sizeof *R);
    for(int i=0;i<N;i++){
        R[i] = xmalloc((size_t)M * sizeof **R);
        for(int j=0;j<M;j++){
            if (scanf("%d", &R[i][j]) != 1){
                fprintf(stderr, "Bad Need at (%d,%d)\n", i, j);
                goto cleanup;
            }
        }
    }

    // Available
    Avail = xmalloc((size_t)M * sizeof *Avail);
    for(int j=0;j<M;j++){
        if (scanf("%d", &Avail[j]) != 1){
            fprintf(stderr, "Bad Available at col %d\n", j);
            goto cleanup;
        }
    }

    Work   = xmalloc((size_t)M * sizeof *Work);
    Finish = xmalloc((size_t)N * sizeof *Finish);

    for(int j=0;j<M;j++) Work[j]   = Avail[j];
    for(int i=0;i<N;i++) Finish[i] = false;

    // Safety check (Banker detection variant)
    int progressed;
    do{
        progressed = 0;
        for(int i=0;i<N;i++){
            if (Finish[i]) continue;
            int ok = 1;
            for(int j=0;j<M;j++){
                if (R[i][j] > Work[j]) { ok = 0; break; }
            }
            if (ok){
                for(int j=0;j<M;j++) Work[j] += A[i][j];
                Finish[i] = true;
                progressed = 1;
            }
        }
    } while(progressed);

    int dead = 0;
    for(int i=0;i<N;i++) if(!Finish[i]) dead++;

    if (!dead){
        puts("NO DEADLOCK");
    } else {
        printf("DEADLOCK on %d process(es):", dead);
        for(int i=0;i<N;i++) if(!Finish[i]) printf(" P%d", i);
        puts("");
    }

cleanup:
    if (A){
        for(int i=0;i<N;i++) if (A[i]) free(A[i]);
        free(A);
    }
    if (R){
        for(int i=0;i<N;i++) if (R[i]) free(R[i]);
        free(R);
    }
    free(Avail);
    free(Work);
    free(Finish);
    return 0;
}
