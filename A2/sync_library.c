#include <pthread.h>
#include <semaphore.h>
#define CACHE_LINE_SIZE_INT 16

/* ---------------------------  LOCKS  ------------------------------ */
/* (a) Lamport's Bakery lock (make sure to avoid false sharing) */
typedef struct bakery_lock_s{
    volatile int * choosing;
    volatile int * ticket;
    int num_threads;
} bakery_lock_t;

void Init_bakery_lock(bakery_lock_t * lock, int num_threads){
    lock->choosing = (int *) calloc(CACHE_LINE_SIZE_INT * num_threads, sizeof(int));
    lock->ticket = (int *) calloc(CACHE_LINE_SIZE_INT * num_threads, sizeof(int));
    lock->num_threads = num_threads;
}

void Acquire_bakery_lock(bakery_lock_t * lock, int tid){
    int maxt, j;
    lock->choosing[tid * CACHE_LINE_SIZE_INT] = 1;
    asm("mfence":::"memory");
    maxt = 0;
    for (j=0; j<lock->num_threads; j++) {
        if (lock->ticket[j * CACHE_LINE_SIZE_INT] > maxt) maxt = lock->ticket[j * CACHE_LINE_SIZE_INT];
    }
    lock->ticket[tid * CACHE_LINE_SIZE_INT] = maxt + 1;
    asm("":::"memory");
    lock->choosing[tid * CACHE_LINE_SIZE_INT] = 0;
    asm("mfence":::"memory");
    for (j=0; j<lock->num_threads; j++) {
        while (lock->choosing[j * CACHE_LINE_SIZE_INT]);
        while (lock->ticket[j * CACHE_LINE_SIZE_INT] &&
                ((lock->ticket[j * CACHE_LINE_SIZE_INT] < lock->ticket[tid * CACHE_LINE_SIZE_INT]) ||
            ((lock->ticket[j * CACHE_LINE_SIZE_INT] == lock->ticket[tid * CACHE_LINE_SIZE_INT]) && (j < tid))));
    }
    return;
}

void Release_bakery_lock(bakery_lock_t *lock, int tid){
    lock->ticket[tid * CACHE_LINE_SIZE_INT] = 0;
    asm("":::"memory");
    return;
}

/* (b) Spin-lock employing cmpxchg instruction of x86 */

int compare_and_set(int oldVal, int newVal, volatile int *lock){
    int oldValOut;
    unsigned char result;
    asm ("lock cmpxchg %4, %1 \n setzb %0"
                 : "=qm"(result), "+m"(*lock), "=a"(oldValOut)
                 : "a"(oldVal), "r"(newVal)
                 : );
    return result;
}

void Acquire_spin_lock(volatile int *lock){
    while(!compare_and_set(0, 1, lock));
}

void Release_spin_lock(volatile int *lock){
    asm("":::"memory");
    *lock = 0;
}

/* (c) Test-and-test-and-set lock employing cmpxchg instruction of x86 */
void Acquire_tts_lock(volatile int * lock){
    while(*lock || !compare_and_set(0, 1, lock));
}

void Release_tts_lock(volatile int * lock){
    asm("":::"memory");
    *lock = 0;
}

/* (d) Ticket lock */
int fetch_and_increment(volatile int * ticket){
    int oldval, newval;
    do{
        oldval = *ticket;
        newval = oldval + 1;
    } while(!compare_and_set(oldval, newval, ticket));
    return oldval;
}

void Acquire_ticket_lock(volatile int * ticket, volatile int * release_count){
    int local_ticket = fetch_and_increment(ticket);
    while (local_ticket != *release_count);
}

void Release_ticket_lock(volatile int * release_count){
    asm("":::"memory");
    (*release_count)++;
}

/* (e) Array lock */
typedef struct array_lock_s {
    volatile int * array;
    volatile int ticket;
    volatile int release_count; 
} array_lock_t;

void Init_array_lock(array_lock_t * lock, int num_threads, int max_N){
    lock->array = (volatile int *) calloc((long long)CACHE_LINE_SIZE_INT * num_threads * max_N , sizeof(int));
    lock->array[0] = 1;
    lock->ticket = 0;
    lock->release_count = 0;
}

void Acquire_array_lock(array_lock_t * lock){
    int local_ticket = fetch_and_increment(&lock->ticket);
    while (!lock->array[local_ticket * CACHE_LINE_SIZE_INT]);
}

void Release_array_lock(array_lock_t * lock){
    asm("":::"memory");
    lock->array[(++(lock->release_count)) * CACHE_LINE_SIZE_INT] = 1;
}

/* Binary Semaphore Lock */
void Acquire_semaphore_lock(sem_t * lock){
    sem_wait(lock);
}

void Release_semaphore_lock(sem_t * lock){
    asm("":::"memory");
    sem_post(lock);
}

/* POSIX mutex lock */
void Acquire_pthread_mutex(pthread_mutex_t *lock) {
    pthread_mutex_lock(lock);
}

/* Release for POSIX mutex */
void Release_pthread_mutex(pthread_mutex_t *lock) {
    pthread_mutex_unlock(lock);
}

/* ---------------------------  BARRIERS  ---------------------------- */
/* (a) Centralized sense-reversing barrier using busy-wait on flag */
typedef struct Barrier_centralized_bw_s{
    pthread_mutex_t lock;
    volatile int counter, flag;
    int num_threads;
} Barrier_centralized_bw_t;

void Barrier_centralized_bw(Barrier_centralized_bw_t * bar){
    static __thread int sense = 0;
    sense = !sense;
    pthread_mutex_lock(&bar->lock);
    bar->counter++;
    if (bar->counter == bar->num_threads){
        bar->counter = 0;
        bar->flag = sense;
        pthread_mutex_unlock(&bar->lock);
    }else{
        pthread_mutex_unlock(&bar->lock);
        while(bar->flag != sense);
    }
}

/* (b) Tree barrier using busy-wait on flags */
typedef struct Barrier_tree_bw_s {
    volatile int **flag;
    int num_threads;
    int levels;
} Barrier_tree_bw_t;

void Init_barrier_tree_bw(Barrier_tree_bw_t * bar, int num_threads){
    bar->num_threads = num_threads;
    bar->num_threads = num_threads;
    bar->flag = (volatile int **)malloc(num_threads * sizeof(volatile int *));

    bar->levels = 0; // log n number of levels
    int temp = num_threads;
    while(temp){
        bar->levels++;
        temp >>= 1;
    }
    for (int i = 0; i < num_threads; i++) {
        bar->flag[i] = (volatile int *)calloc(bar->levels, sizeof(int));  // Initialize all flags to 0
    }
}

void Barrier_tree_bw(Barrier_tree_bw_t * bar, int tid){
    unsigned int i = 0, mask = 1;

    while ((mask & tid) != 0) {
        while (!bar->flag[tid][i]); // Busy-wait for the flag to be set
        bar->flag[tid][i] = 0;
        ++i;
        mask <<= 1;
    }

    if (tid < (bar->num_threads - 1)) {
        bar->flag[tid + mask][i] = 1;  // Set flag for partner at next level
        while (!bar->flag[tid][bar->levels - 1]); 
        bar->flag[tid][bar->levels - 1] = 0;   // Reset the bar->levels-1 flag after completion
    }

    for (mask >>= 1; mask > 0; mask >>= 1) {
        bar->flag[tid - mask][bar->levels - 1] = 1; // Signal partner threads at each level
    }
}

/* (c) Centralized barrier using POSIX condition variable */
typedef struct Barrier_centralized_cv_s{
    pthread_mutex_t lock;
    pthread_cond_t cv;
    int count;
    int sense;
    int num_threads;
} Barrier_centralized_cv_t;

void Init_Barrier_centralized_cv(Barrier_centralized_cv_t* bar, int num_threads) {
    pthread_mutex_init(&bar->lock, NULL);
    pthread_cond_init(&bar->cv, NULL);
    bar->count = 0;
    bar->num_threads = num_threads;
    bar->sense = 0;
}

void Barrier_centralized_cv(Barrier_centralized_cv_t *bar){
    pthread_mutex_lock(&bar->lock);
    int my_sense = bar->sense;
    bar->count++;
    if (bar->count == bar->num_threads) {
        bar->count = 0;
        bar->sense = !bar->sense;
        pthread_cond_broadcast(&bar->cv);
    } else {
        while (bar->sense == my_sense) {
            pthread_cond_wait(&bar->cv, &bar->lock);
        }
    }
    pthread_mutex_unlock(&bar->lock);
}

/* (d) Tree barrier using POSIX condition variable */
typedef struct Barrier_tree_cv_s {
    pthread_mutex_t **lock;
    pthread_cond_t ** cv;
    int ** flag;
    int num_threads;
    int levels;
} Barrier_tree_cv_t;

void Init_barrier_tree_cv(Barrier_tree_cv_t * bar, int num_threads){
    bar->num_threads = num_threads;
    bar->num_threads = num_threads;
    bar->lock = (pthread_mutex_t **)malloc(num_threads * sizeof(pthread_mutex_t *));
    bar->cv = (pthread_cond_t **)malloc(num_threads * sizeof(pthread_cond_t *));
    bar->flag = (int **)malloc(num_threads * sizeof(int *));

    bar->levels = 0; // log n number of levels
    int temp = num_threads;
    while(temp){
        bar->levels++;
        temp >>= 1;
    }
    for (int i = 0; i < num_threads; i++) {
        bar->lock[i] = (pthread_mutex_t *)malloc(bar->levels * sizeof(pthread_mutex_t)); 
        bar->cv[i] = (pthread_cond_t *)malloc(bar->levels * sizeof(pthread_cond_t)); 
        bar->flag[i] = (int *)malloc(bar->levels * sizeof(int));  // Initialize all flags to 0
        for (int j = 0; j < bar->levels; j++) {
            pthread_mutex_init(&bar->lock[i][j], NULL);
            pthread_cond_init(&bar->cv[i][j], NULL);
            bar->flag[i][j] = 0;
        }
    }
}

void Barrier_tree_cv(Barrier_tree_cv_t * bar, int tid){
    unsigned int i = 0, mask = 1;
    while ((mask & tid) != 0) {
        pthread_mutex_lock(&bar->lock[tid][i]);        
        while (!bar->flag[tid][i]) {
            pthread_cond_wait(&bar->cv[tid][i], &bar->lock[tid][i]); // replace busy-wait by cond wait
        }
        bar->flag[tid][i] = 0;
        pthread_mutex_unlock(&bar->lock[tid][i]);
        ++i;
        mask <<= 1;
    }

    if (tid < (bar->num_threads - 1)) {
        pthread_mutex_lock(&bar->lock[tid + mask][i]);
        bar->flag[tid + mask][i] = 1;  
        pthread_mutex_unlock(&bar->lock[tid + mask][i]);

        pthread_cond_signal(&bar->cv[tid + mask][i]);  

        pthread_mutex_lock(&bar->lock[tid][bar->levels - 1]);
        while(!bar->flag[tid][bar->levels - 1]) {
            pthread_cond_wait(&bar->cv[tid][bar->levels - 1], &bar->lock[tid][bar->levels - 1]);
        }
        bar->flag[tid][bar->levels - 1] = 0;   // Reset the bar->levels-1 flag after completion
        pthread_mutex_unlock(&bar->lock[tid][bar->levels - 1]);
        // while (!bar->flag[tid][bar->levels - 1]); 
        // bar->flag[tid][bar->levels - 1] = 0;   // Reset the bar->levels-1 flag after completion
    }

    for (mask >>= 1; mask > 0; mask >>= 1) {
        pthread_mutex_lock(&bar->lock[tid - mask][bar->levels - 1]);
        bar->flag[tid - mask][bar->levels - 1] = 1; // Signal partner threads at each level
        pthread_cond_signal(&bar->cv[tid - mask][bar->levels - 1]); // Signal partner threads at each level
        pthread_mutex_unlock(&bar->lock[tid - mask][bar->levels - 1]);
        // bar->flag[tid - mask][bar->levels - 1] = 1; // Signal partner threads at each level
    }
}

/* (e) POSIX barrier interface (pthread_barrier_wait) */
void Barrier_pthread (pthread_barrier_t* barrier)
{
    pthread_barrier_wait(barrier);
}