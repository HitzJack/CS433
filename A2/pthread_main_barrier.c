#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>
#include <pthread.h>
#include "sync_library.c"

#ifdef CENTRALIZED_BW_BARRIER
    Barrier_centralized_bw_t bar = {PTHREAD_MUTEX_INITIALIZER};
    #define barrier Barrier_centralized_bw(&bar);
#elif TREE_BW_BARRIER
    Barrier_tree_bw_t bar;
    #define barrier Barrier_tree_bw(&bar, tid);
#elif CENTRALIZED_CV_BARRIER
    Barrier_centralized_cv_t bar;
    #define barrier Barrier_centralized_cv(&bar);
#elif TREE_CV_BARRIER
    Barrier_tree_cv_t bar;
    #define barrier Barrier_tree_cv(&bar, tid);
#else
    // Default to pthread barrier
    pthread_barrier_t bar;
    #define barrier Barrier_pthread(&bar);
#endif

struct timeval tv0, tv1;
struct timezone tz0, tz1;

#define N 1e6
int global = 0;

void *benchmark(void *arg) {
    int i;
    int tid = *(int *) arg;
    for (i = 0; i < N; i++) {
        barrier
    }
    return NULL;
}

int main(int argc, char * argv[]){
    if (argc != 2){
        printf("Please pass number of threads in input\n");
        return 0;
    }
    int t = atoi(argv[1]);
    pthread_t threads[t];
    int * tid = (int*)malloc(t*sizeof(int));
    int i;
    
    // Initialize barrier
    #ifdef CENTRALIZED_BW_BARRIER
        bar.counter = 0;
        bar.flag = 0;
        bar.num_threads = t;
    #elif TREE_BW_BARRIER
        Init_barrier_tree_bw(&bar, t);
    #elif CENTRALIZED_CV_BARRIER
        Init_Barrier_centralized_cv(&bar, t);
    #elif TREE_CV_BARRIER
        Init_barrier_tree_cv(&bar, t);
    #else
        // Default to pthread barrier
        pthread_barrier_init(&bar, NULL, t);
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

    gettimeofday(&tv1, &tz1);

    // Destroying the barriers
    #ifdef CENTRALIZED_BW_BARRIER
        pthread_mutex_destroy(&bar.lock);
    #elif TREE_BW_BARRIER
        for (int i = 0; i < bar.num_threads; i++) {
            free((void *)bar.flag[i]);
        }
        free((void *)bar.flag);
    #elif CENTRALIZED_CV_BARRIER
        pthread_mutex_destroy(&bar.lock);
        pthread_cond_destroy(&bar.cv);
    #elif TREE_CV_BARRIER
        for (int i = 0; i < bar.num_threads; i++) {
            free((void *)bar.lock[i]);
            free((void *)bar.cv[i]);
            free((void *)bar.flag[i]);
        }
        free((void *)bar.lock);
        free((void *)bar.cv);
        free((void *)bar.flag);
    #else
        // Default to pthread barrier
        pthread_barrier_destroy(&bar);
    #endif
    
    long int time_taken = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);
    printf("Time Taken: %ld microseconds\n", time_taken);
    return 0;
}