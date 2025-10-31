#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <time.h>   // nanosleep

// sleep mili-giây an toàn chuẩn POSIX
static void msleep(int ms){
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;

static void* t1(void* arg){
    (void)arg;                    // tránh cảnh báo unused
    pthread_mutex_lock(&A);
    msleep(100);                  // 100ms để tạo điều kiện đua
    pthread_mutex_lock(&B);       // sẽ bị kẹt ở đây khi t2 giữ B
    puts("t1 got both A and B");  // thực tế không bao giờ tới
    pthread_mutex_unlock(&B);
    pthread_mutex_unlock(&A);
    return NULL;
}

static void* t2(void* arg){
    (void)arg;
    pthread_mutex_lock(&B);
    msleep(100);
    pthread_mutex_lock(&A);       // sẽ bị kẹt ở đây khi t1 giữ A
    puts("t2 got both B and A");
    pthread_mutex_unlock(&A);
    pthread_mutex_unlock(&B);
    return NULL;
}

int main(void){
    pthread_t th1, th2;
    pthread_create(&th1, NULL, t1, NULL);
    pthread_create(&th2, NULL, t2, NULL);
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);
    return 0;
}
