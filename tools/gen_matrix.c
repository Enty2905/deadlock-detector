#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static unsigned long long s = 2685821657736338717ULL;
static unsigned rndu(void){ s ^= s<<7; s ^= s>>9; return (unsigned)(s & 0xffffffffu); }
static void sseed(unsigned long long x){ if(x==0) x=1; s=x; }
static int rndint(int lo,int hi){ unsigned r=rndu(); int span=hi-lo+1; return lo + (int)(r% (unsigned)span); }

int main(int argc,char**argv){
    if(argc<3){
        fprintf(stderr,"Usage: %s N M [--seed S] [--mode ok|dead]\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    unsigned long long seed = 1;
    const char* mode = "ok";

    for(int i=3;i<argc;i++){
        if(!strcmp(argv[i],"--seed") && i+1<argc) seed = strtoull(argv[++i],NULL,10);
        else if(!strcmp(argv[i],"--mode") && i+1<argc) mode = argv[++i];
    }
    if(N<=0 || M<=0){ fprintf(stderr,"Bad N/M\n"); return 1; }
    sseed(seed);

    /* cấp phát */
    int **A = (int**)malloc((size_t)N*sizeof(*A));
    int **R = (int**)malloc((size_t)N*sizeof(*R));
    int  *Avail = (int*)malloc((size_t)M*sizeof(*Avail));
    if(!A||!R||!Avail){ perror("malloc"); return 1; }

    for(int i=0;i<N;i++){
        A[i] = (int*)malloc((size_t)M*sizeof(**A));
        R[i] = (int*)malloc((size_t)M*sizeof(**R));
        if(!A[i]||!R[i]){ perror("malloc"); return 1; }
    }

    /* sinh dữ liệu đơn giản, giới hạn tài nguyên 0..3 */
    for(int j=0;j<M;j++) Avail[j] = rndint(1,3);

    if(!strcmp(mode,"ok")){
        for(int i=0;i<N;i++){
            for(int j=0;j<M;j++){
                A[i][j] = rndint(0,1);
                int need_max = Avail[j] + A[i][j];   /* đảm bảo khả thi */
                if(need_max<0) need_max=0;
                R[i][j] = rndint(0, need_max);
            }
        }
    }else{ /* mode dead: cố tình tạo bế tắc bằng vòng nhu cầu */
        for(int i=0;i<N;i++){
            for(int j=0;j<M;j++){
                A[i][j] = 0;
                R[i][j] = 0;
            }
        }
        /* tạo chu trình nhu cầu kiểu: P0 cần R0, P1 cần R1, ...; Avail=0 để không ai start */
        for(int j=0;j<M;j++) Avail[j] = 0;
        int K = (N<M)?N:M;
        if(K<2) K=2;
        for(int i=0;i<K;i++){
            int j = i%M;
            R[i][j] = 1;
        }
    }

    /* in ra: N M, ma trận A (N dòng), ma trận R (N dòng), rồi Avail (1 dòng) */
    printf("%d %d\n", N, M);
    for(int i=0;i<N;i++){
        for(int j=0;j<M;j++){
            printf("%d%c", A[i][j], (j+1<M)?' ':'\n');
        }
    }
    for(int i=0;i<N;i++){
        for(int j=0;j<M;j++){
            printf("%d%c", R[i][j], (j+1<M)?' ':'\n');
        }
    }
    for(int j=0;j<M;j++){
        printf("%d%c", Avail[j], (j+1<M)?' ':'\n');
    }

    for(int i=0;i<N;i++){ free(A[i]); free(R[i]); }
    free(A); free(R); free(Avail);
    return 0;
}
