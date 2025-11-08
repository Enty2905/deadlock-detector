#include "util.h"
#include <pthread.h>
#include <stdio.h>

static pthread_mutex_t M1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t M2 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t M3 = PTHREAD_MUTEX_INITIALIZER;

static void* tA(void* arg){
    (void)arg;
    pthread_mutex_lock(&M1);
    msleep(50);
    pthread_mutex_lock(&M2);
    msleep(20);
    pthread_mutex_unlock(&M2);
    pthread_mutex_unlock(&M1);
    return NULL;
}
static void* tB(void* arg){
    (void)arg;
    pthread_mutex_lock(&M2);
    msleep(50);
    pthread_mutex_lock(&M3);
    msleep(20);
    pthread_mutex_unlock(&M3);
    pthread_mutex_unlock(&M2);
    return NULL;
}
static void* tC(void* arg){
    (void)arg;
    pthread_mutex_lock(&M3);
    msleep(50);
    pthread_mutex_lock(&M1);
    msleep(20);
    pthread_mutex_unlock(&M1);
    pthread_mutex_unlock(&M3);
    return NULL;
}
int main(void){
    pthread_t a,b,c;
    pthread_create(&a,NULL,tA,NULL);
    pthread_create(&b,NULL,tB,NULL);
    pthread_create(&c,NULL,tC,NULL);
    pthread_join(a,NULL);
    pthread_join(b,NULL);
    pthread_join(c,NULL);
    return 0;
}
