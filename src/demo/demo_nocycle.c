#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <time.h>

static void msleep(int ms){ struct timespec ts={ms/1000,(long)(ms%1000)*1000000L}; nanosleep(&ts,NULL); }

static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

static void* t1(void* _){ (void)_;
    pthread_mutex_lock(&A); msleep(50);
    pthread_mutex_lock(&B); msleep(50);
    pthread_mutex_unlock(&B); pthread_mutex_unlock(&A);
    puts("t1 done"); return NULL;
}
static void* t2(void* _){ (void)_;
    pthread_mutex_lock(&A); msleep(50);
    pthread_mutex_lock(&B); msleep(50);
    pthread_mutex_unlock(&B); pthread_mutex_unlock(&A);
    puts("t2 done"); return NULL;
}

int main(void){
    pthread_t x,y; pthread_create(&x,NULL,t1,NULL); pthread_create(&y,NULL,t2,NULL);
    pthread_join(x,NULL); pthread_join(y,NULL);
    return 0;
}
