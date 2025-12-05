#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "util.h"
 
void *xmalloc(size_t n){
    void *p = malloc(n);
    if(!p){ perror("malloc"); exit(1); }
    return p;
}

void *xcalloc(size_t n, size_t sz){
    void *p = calloc(n, sz);
    if(!p){ perror("calloc"); exit(1); }
    return p;
}

void *xrealloc(void *p, size_t n){
    void *q = realloc(p, n);
    if(!q){ perror("realloc"); exit(1); }
    return q;
}

void msleep(unsigned ms){
    if(ms == 0) return;
    struct timespec ts;
    ts.tv_sec  = ms / 1000u;
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
        /* tiếp tục ngủ phần còn lại khi bị tín hiệu đánh thức */
    }
}
