#include "util.h"
#include <pthread.h>
#include <stdio.h>

static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

static void* t1(void* arg){
    (void)arg;
    pthread_mutex_lock(&A);
    msleep(100);
    pthread_mutex_lock(&B);
    msleep(50);
    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return NULL;
}
static void* t2(void* arg){
    (void)arg;
    pthread_mutex_lock(&B);
    msleep(100);
    pthread_mutex_lock(&A);
    msleep(50);
    pthread_mutex_unlock(&A);
    pthread_mutex_unlock(&B);
    return NULL;
}
int main(void){
    pthread_t x,y;
    pthread_create(&x,NULL,t1,NULL);
    pthread_create(&y,NULL,t2,NULL);
    pthread_join(x,NULL);
    pthread_join(y,NULL);
    return 0;
}
