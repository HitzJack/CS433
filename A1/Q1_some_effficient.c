#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
#include <assert.h>

#define TEST_SIZES 1
#define SIZES {20, 22}
#define TEST_THREADS 3
#define THREADS {1, 8, 4, 8, 10}
#define ITERS 100
#define inf 100000000
typedef unsigned int uint;

/* Utilities */
typedef struct Timer{
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;
} Timer;
void startTimer(Timer* timer){
    gettimeofday(&(timer->tv0), &(timer->tz0));
}
void stopTimer(Timer* timer){
    gettimeofday(&(timer->tv1), &(timer->tz1));
}
long int printDuration(Timer *timer){
    long int us = (timer->tv1.tv_sec-timer->tv0.tv_sec)*1000000+(timer->tv1.tv_usec-timer->tv0.tv_usec);
    printf("Time: %ld microseconds\n", us);
    return us;
}
uint tsp_basic(uint n, uint ** w, uint ** dp){
    uint temp;
    for (uint i =0; i < n-1; i++){
        dp[i][0] = w[n-1][i];
    }
    int mask_size = (1 << (n-1));
    for (uint i = 1; i < mask_size; i++){
        for (uint j = 0; j < n-1; j++){
            if (i & (1<<j)){ // j in the set
                continue;
            }
            for (uint k = 0; k < n-1; k++){
                if (i & (1<<k)){
                    temp = dp[k][i ^ (1 << k)] + w[k][j];
                    if (temp < dp[j][i]){
                        dp[j][i] = temp;
                    }
                }
            }
        }
    }
    uint mn = inf;
    uint lastIndex = 0;
    for (uint i = 0; i < n-1; i++){
        if (dp[i][(mask_size - 1) ^ (1<<i)] + w[i][n-1] < mn){
            mn = dp[i][(mask_size - 1) ^ (1<<i)] + w[i][n-1];
            lastIndex = i;
            temp = (mask_size - 1) ^ (1<<i);
        }
    }
    return mn;
}
int verify_tsp(uint n, uint ** w, uint ** dp, uint * tour){
    for (int i = 0; i < n-1; i++){
        for (int j = 0; j < (1 << (n-1)); j++){
            dp[i][j] = inf;
        }
    }
    uint min_weight = tsp_basic(n, w, dp);
    uint t_weight = 0;
    int prev = n;
    for (int i = 0; i < n; i++){
        t_weight += w[prev-1][tour[i]-1];
        prev = tour[i];
    }
    if (t_weight != min_weight){
        printf("T_weight = %d, min_weight = %d\n", t_weight, min_weight);
        fflush(stdout);
    }
    return t_weight == min_weight;
}

uint tsp_sequential(uint n, uint ** w, uint ** dp, uint ** prev, uint * tour){
    uint temp, k;
    uint i, j;
    for (uint i =0; i < n-1; i++){
        dp[i][0] = w[n-1][i];
        prev[i][0] = n-1;
    }
    int mask_size = (1 << (n-1));
    for (i = 1; i < mask_size; i++){
        for (j = 0; j < n-1; j++){
            if (i & (1<<j)){ // j in the set
                continue;
            }
            for (k = 0; k < n-1; k++){
                if (i & (1<<k)){
                    temp = dp[k][i ^ (1 << k)] + w[k][j];
                    if (temp < dp[j][i]){
                        dp[j][i] = temp;
                        prev[j][i] = k;
                    }
                }
            }
        }
    }
    uint mn = inf;
    uint lastIndex = 0;
    for (uint i = 0; i < n-1; i++){
        if (dp[i][(mask_size - 1) ^ (1<<i)] + w[i][n-1] < mn){
            mn = dp[i][(mask_size - 1) ^ (1<<i)] + w[i][n-1];
            lastIndex = i;
            temp = (mask_size - 1) ^ (1<<i);
        }
    }
    tour[0] = lastIndex + 1;

    for (uint i = 1; i < n; i++){
        tour[i] = prev[lastIndex][temp] + 1;
        lastIndex = prev[lastIndex][temp];
        temp ^= (1 << (tour[i]-1));
    }

    return mn;
}


/* Main Code */
uint tsp(uint n, uint ** w, uint ** dp, uint ** prev, uint * tour, uint num_threads){
    if (num_threads == 1){
        return tsp_sequential(n, w, dp, prev, tour);
    }
    uint temp;
    uint i;
    uint j, k;
    for (uint i =0; i < n-1; i++){
        dp[i][0] = w[n-1][i];
        prev[i][0] = n-1;
    }
    int mask_size = (1 << (n-1));
    #pragma omp parallel num_threads (num_threads) private(temp, k, j, i)
    for (i = 1; i < mask_size; i++){
        #pragma omp for
        for (j = 0; j < n-1; j++){
            if (i & (1<<j)){ // j in the set
                continue;
            }
            for (k = 0; k < n-1; k++){
                if (i & (1<<k)){
                    temp = dp[k][i ^ (1 << k)] + w[k][j];
                    if (temp < dp[j][i]){
                        dp[j][i] = temp;
                        prev[j][i] = k;
                    }
                }
            }
        }
    }
    uint mn = inf;
    uint lastIndex = 0;
    for (uint i = 0; i < n-1; i++){
        if (dp[i][(mask_size - 1) ^ (1<<i)] + w[i][n-1] < mn){
            mn = dp[i][(mask_size - 1) ^ (1<<i)] + w[i][n-1];
            lastIndex = i;
            temp = (mask_size - 1) ^ (1<<i);
        }
    }
    tour[0] = lastIndex + 1;
    int my_weight = w[lastIndex][n-1];

    for (uint i = 1; i < n; i++){
        tour[i] = prev[lastIndex][temp] + 1;
        my_weight += w[lastIndex][tour[i]-1];
        lastIndex = prev[lastIndex][temp];
        temp ^= (1 << (tour[i]-1));
    }

    return mn;
}

int main(){
    int nvalues[] = SIZES;
    int tvalues[] = THREADS;
    Timer t;
    FILE * analysis_csv = fopen("analysis.csv", "w");
    if (analysis_csv == NULL){
        perror("CLOSE analysis.csv\n");
        return -1;
    }
    fprintf(analysis_csv, "Graph_size, num_threads, iteration, time_taken(us), min_weight\n");
    for (int i = 0; i < TEST_SIZES; i++){
        int n = nvalues[i]; // value of n
        uint ** edge_weights = (uint **) malloc(n * sizeof(uint *));
        for (uint i = 0; i < n; i++){
            edge_weights[i] = (uint *) malloc((n) * sizeof(uint));
            edge_weights[i][i] = 0;
        }
        uint x;
        uint * tour = (uint *) malloc(n * sizeof(uint));
        uint ** dp = (uint **) malloc((n-1) * sizeof(uint *));
        uint ** prev = (uint **) malloc((n-1) * sizeof(uint *));
        for (uint i = 0; i < n-1; i++){
            dp[i] = (uint *) malloc((1 << (n-1)) * sizeof(uint));
            prev[i] = (uint *) malloc((1 << (n-1)) * sizeof(uint));
        }

        for (int j = 0; j < ITERS; j++){
            // Initialize
            for (uint i = 0; i < n; i++){
                for (uint j = i+1; j < n; j++){
                    x = rand() % 10;
                    edge_weights[i][j] = x;
                    edge_weights[j][i] = x;
                }
            }

            for (int k = 0; k < TEST_THREADS; k++){
                // setup 
                for (uint i = 0; i < n-1; i++){
                    for (uint j = 0; j < (1 << (n-1)); j++){
                        dp[i][j] = inf;
                    }
                }
                int num_threads = tvalues[k];

                printf("RUNNING :: SIZE = %d :: THREADS = %d :: ITERATION = %d :: ", n, num_threads, j+1);
                fflush(stdout);

                startTimer(&t);
                uint min_weight = tsp(n, edge_weights, dp, prev, tour, num_threads);
                stopTimer(&t);
                if (!verify_tsp(n, edge_weights, dp, tour)){
                    perror("PANIC :: INVALID TOUR\n");
                    return -1;
                }
                long int d = printDuration(&t);
                fprintf(analysis_csv, "%d, %d, %d, %ld, %d\n", n, num_threads, (j+1), d, min_weight);
            }
        }
        for (int i = 0; i < n-1; i++){
            free(dp[i]);
            free(prev[i]);
            free(edge_weights[i]);
        }
        free(edge_weights[n-1]);

        free(edge_weights);
        free(dp);
        free(prev);
        free(tour);
    }
    fclose(analysis_csv);
    printf("COMPLETED\n");
    return 0;
}