#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void *t1(void *arg) {
    (void)arg; // tránh warning unused parameter
    pthread_mutex_lock(&m1);
    sleep(1);
    pthread_mutex_lock(&m2);
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

void *t2(void *arg) {
    (void)arg;
    pthread_mutex_lock(&m2);
    sleep(1);
    pthread_mutex_lock(&m1);
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, t1, NULL);
    pthread_create(&thread2, NULL, t2, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Program finished.\n");
    return 0;
}

