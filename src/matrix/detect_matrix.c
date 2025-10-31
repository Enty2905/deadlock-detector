#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        perror("malloc");
        exit(1);
    }
    return p;
}
int main(int argc, char **argv) {
    if (argc == 2) {
        if (!freopen(argv[1], "r", stdin)) {
            perror("freopen");
            return 1;
        }
    }
    int N, M;
    if (scanf("%d %d", &N, &M) != 2 || N <= 0 || M <= 0) {
        fprintf(stderr, "Bad N M\n");
        return 1;
    }
    int **A = xmalloc(N * sizeof *A);  // Allocation
    int **R = xmalloc(N * sizeof *R);  // Request/Need
    for (int i = 0; i < N; i++) {
        A[i] = xmalloc(M * sizeof **A);
        for (int j = 0; j < M; j++)
            if (scanf("%d", &A[i][j]) != 1)
                return 1;
    }
    for (int i = 0; i < N; i++) {
        R[i] = xmalloc(M * sizeof **R);
        for (int j = 0; j < M; j++)
            if (scanf("%d", &R[i][j]) != 1)
                return 1;
    }
    int *Avail = xmalloc(M * sizeof *Avail);
    for (int j = 0; j < M; j++)
        if (scanf("%d", &Avail[j]) != 1)
            return 1;
    int *Work = xmalloc(M * sizeof *Work);
    bool *Finish = xmalloc(N * sizeof *Finish);
    for (int j = 0; j < M; j++) Work[j] = Avail[j];
    for (int i = 0; i < N; i++) Finish[i] = false;
    int progressed;
    do {
        progressed = 0;
        for (int i = 0; i < N; i++) {
            if (Finish[i])
                continue;
            int ok = 1;
            for (int j = 0; j < M; j++)
                if (R[i][j] > Work[j]) {
                    ok = 0;
                    break;
                }
            if (ok) {
                for (int j = 0; j < M; j++) Work[j] += A[i][j];
                Finish[i] = true;
                progressed = 1;
            }
        }
    } while (progressed);
    int dead = 0;
    for (int i = 0; i < N; i++)
        if (!Finish[i])
            dead++;
    if (dead == 0)
        puts("NO DEADLOCK");
    else {
        printf("DEADLOCK on %d process(es):", dead);
        for (int i = 0; i < N; i++)
            if (!Finish[i])
                printf(" P%d", i);
        putchar('\n');
    }
    return 0;
}
