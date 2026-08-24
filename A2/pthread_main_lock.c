#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include "sync_library.c"

// #define BAKERY_LOCK
// #define SPIN_LOCK
// #define TTS_LOCK
// #define TICKET_LOCK
// #define ARRAY_LOCK
// #define SEMAPHORE_LOCK

#ifdef BAKERY_LOCK
    bakery_lock_t lock;
    #define Acquire Acquire_bakery_lock(&lock, tid);
    #define Release Release_bakery_lock(&lock, tid);
#elif SPIN_LOCK
    volatile int lock = 0;
    #define Acquire Acquire_spin_lock(&lock);
    #define Release Release_spin_lock(&lock);
#elif TTS_LOCK
    volatile int lock = 0;
    #define Acquire Acquire_tts_lock(&lock);
    #define Release Release_tts_lock(&lock);
#elif TICKET_LOCK
    volatile int ticket = 0;
    volatile int release_count = 0;
    #define Acquire Acquire_ticket_lock(&ticket, &release_count);
    #define Release Release_ticket_lock(&release_count);
#elif ARRAY_LOCK
    array_lock_t lock;
    #define Acquire Acquire_array_lock(&lock);
    #define Release Release_array_lock(&lock);
#elif SEMAPHORE_LOCK
    sem_t lock;
    #define Acquire Acquire_semaphore_lock(&lock);
    #define Release Release_semaphore_lock(&lock);
#else
    // Default to pthread mutex
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    #define Acquire Acquire_pthread_mutex(&lock);
    #define Release Release_pthread_mutex(&lock);
#endif

struct timeval tv0, tv1;
struct timezone tz0, tz1;

#define N 1e7
int x = 0, y = 0;

void *benchmark(void *arg) {
    int i;
    int tid = *((int *)arg);
    for (i = 0; i < N; i++) {
        Acquire
        assert (x == y);
        x = y + 1;
        y++;
        Release
    }
    return NULL;
}

int main(int argc, char * argv[]){
    if (argc != 2){
        printf("Please pass number of threads in input\n");
        return 0;
    }
    int t = atoi(argv[1]);
    int * tid = (int*)malloc(t*sizeof(int));
    pthread_t threads[t];
    int i;
    // Initialize lock
    #ifdef BAKERY_LOCK
        Init_bakery_lock(&lock, t);
    #elif ARRAY_LOCK
        Init_array_lock(&lock, t, N);
    #elif SEMAPHORE_LOCK
        sem_init(&lock, 0, 1);
    #endif
    gettimeofday(&tv0, &tz0);

    // Create threads
    for (int i = 0; i < t; i++) {
        tid[i] = i;
        pthread_create(&threads[i], NULL, benchmark, &tid[i]);
    }

    // Join threads
    for (int i = 0; i < t; i++) {
        pthread_join(threads[i], NULL);
    }
    assert(x == N*t);

    gettimeofday(&tv1, &tz1);

    // Destroy the locks
    #ifdef BAKERY_LOCK
        free((void *)lock.choosing);
        free((void *)lock.ticket);
    #elif ARRAY_LOCK
        free((void *)lock.array);
    #elif SEMAPHORE_LOCK
        sem_destroy(&lock);
    #endif
    long int time_taken = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);
    printf("Time Taken: %ld microseconds\n", time_taken);
    return 0;
}