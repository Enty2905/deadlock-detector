#ifndef UTIL_H
#define UTIL_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
static inline void* xmalloc(size_t n){ void* p=malloc(n); if(!p){perror("malloc"); exit(1);} return p; }
static inline void* xcalloc(size_t c,size_t n){ void* p=calloc(c,n); if(!p){perror("calloc"); exit(1);} return p; }
static inline void* xrealloc(void* old,size_t n){ void* p=realloc(old,n); if(!p){perror("realloc"); exit(1);} return p; }
#endif
