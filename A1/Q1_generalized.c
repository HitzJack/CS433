#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
#define ANALYSIS
// #define DEBUG
#define inf 1e9
typedef unsigned int uint;
#ifdef ANALYSIS
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;
#endif
#define SEGFENCE printf("Reached LINE %d\n", __LINE__); fflush(stdout);
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
    printf("Done\n");fflush(stdout);
    uint t_weight = 0;
    int prev = n;
    for (int i = 0; i < n; i++){
        if (tour[i] > n || tour[i] <= 0) {
            printf("INVALID VALUE %d in tour\n", tour[i]);fflush(stdout);
            return 0;
        }
        t_weight += w[prev-1][tour[i]-1];
        prev = tour[i];
    }
    if (t_weight != min_weight){
        printf("Found Tour weight = %d, min_weight_possible = %d\n", t_weight, min_weight);
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
void generate_bitmasks(uint * arr, uint * llimit, uint * rlimit, int n){
    int l = (1 << n) - 1;
    arr[0] = 0;
    llimit[1] = 1;
    int t1 = 0, t2, nb = 1, curr = 1;
    for (int i = 1; i < l; i++){
        arr[i] = curr;
        // next
        t1 = 0;t2 = 0;
        while(t2 <= n+1){
            if (curr & 1){
                t1 = (t1 << 1) | 1;
            }else if (t1){
                curr = ((curr|1) << t2)|(t1 >> 1);
                break;
            }
            t2++;
            curr >>= 1;
        }

        if (curr >= l){
            rlimit[nb++] = i;
            llimit[nb] = i+1;
            curr = (1 << nb) - 1;
        }
    }
}

uint solve(int n, int num_threads, uint ** A, uint * tour){
    typedef uint mask;
    const int lm = (1 << (n-1));
    mask * masks = (mask *) malloc( lm * sizeof(mask));
    uint llimit[n];
    uint rlimit[n];
    uint ** dp = (uint **) malloc(lm * sizeof(uint *));
    uint ** prev = (uint **) malloc(lm * sizeof(uint *));
    for (int i = 0; i < lm; i++){
        dp[i] = (uint *)malloc((n-1)*sizeof(uint));
        prev[i] = (uint *)malloc((n-1)*sizeof(uint));
        for (int j = 0; j < n-1; j++){
            dp[i][j] = inf;
        }
    }

    #ifdef ANALYSIS
        gettimeofday(&tv0, &tz0);
    #endif
    generate_bitmasks(masks, llimit, rlimit, n-1);
    uint i, j, k, l, tj, tk, tl;
    for (uint i =0; i < n-1; i++){
        dp[0][i] = A[n-1][i];
        prev[0][i] = n-1;
    }
    
    #pragma omp parallel num_threads(num_threads) private(i, j, k, l, tj, tk, tl)
    for (i = 1; i < n-1; i++){
        #pragma omp for
        for (j = llimit[i]; j <= rlimit[i]; j++){
            tj = ~masks[j];
            for (k = __builtin_ctz(tj); k < n-1; tj &= tj-1, k = __builtin_ctz(tj)){
                tk = masks[j];
                for (l = __builtin_ctz(tk); l < n-1; tk &= tk-1, l = __builtin_ctz(tk)){
                    tl = dp[masks[j] ^ (1 << l)][l] + A[l][k];
                    if (tl < dp[masks[j]][k]){
                        dp[masks[j]][k] = tl;
                        prev[masks[j]][k] = l;
                    }
                }
            }
        }
    }


    uint mn = inf, temp;
    uint lastIndex = -1;
    for (uint i = 0; i < n-1; i++){
        if (dp[(lm - 1) ^ (1<<i)][i] + A[i][n-1] < mn){
            mn = dp[(lm - 1) ^ (1<<i)][i] + A[i][n-1];
            lastIndex = i;
            temp = (lm - 1) ^ (1<<i);
        }
    }
    tour[0] = lastIndex + 1;
    for (uint i = 1; i < n; i++){
        tour[i] = prev[temp][lastIndex] + 1;
        lastIndex = prev[temp][lastIndex];
        temp ^= (1 << (tour[i]-1));
    }

    #ifdef ANALYSIS
        gettimeofday(&tv1, &tz1);
    #endif
    return mn;
}

int main(int argc, char *argv[]){
    if (argc != 4){
        perror("Need 3 arguments: input_filename output_filename num_threads\n");
        return -1;
    }
    int tries = atoi(argv[3]);
    FILE * input_file = fopen(argv[1], "r");
    FILE * output_file = fopen(argv[2], "w");

    for (int try = 0; try < tries; try++){
    // printf("%lu %lu\n", sizeof(uint), sizeof(long uint));
    // Read arguments
    int num_threads;
    num_threads = atoi(argv[3]);
    if (input_file == NULL){
        perror("Could not open input file\n");
    }
    if (output_file == NULL){
        perror("Could not open output file\n");
    }

    // Read input and allocate necessary memory
    int n;
    int temp;
    temp = fscanf(input_file, "%d", &n);
    if (num_threads > n){
        num_threads = n;
    }
    uint ** A = (uint **) malloc(n * sizeof(uint *));
    uint * tour = (uint *) malloc(n * sizeof(uint));

    /* INITIALIZATION */
    #ifdef ANALYSIS
        #ifdef DEBUG
            fprintf(output_file, "INPUT A\n");
        #endif
        for (int i = 0; i < n; i++){
            A[i] = (uint *) malloc((n) * sizeof(uint));
        }
        for (int i = 0; i < n; i++){
            A[i][i] = 0;
            for (int j = i+1; j < n; j++){
                A[i][j] = rand() % 100 + 1;
                A[j][i] = A[i][j];
                #ifdef DEBUG
                    fprintf(output_file, "%u%c", A[i][j], (j == n-1)? '\n': ' ');
                #endif
            }
        }
    #else
        for (int i = 0; i < n; i++){
            A[i] = (uint *) malloc(n * sizeof(uint));
        }
        for (int i = 0; i < n; i++){
            A[i][i] = 0;
            for (int j = i+1; j < n; j++){
                temp = fscanf(input_file, "%d", &A[i][j]);
                A[j][i] = A[i][j];
            }
        }
    #endif

    #ifdef ANALYSIS
    printf("INITIALIZED\n");
    fflush(stdout);
        gettimeofday(&tv0, &tz0);
    #endif
    uint answer = solve(n, num_threads, A, tour);

    #ifdef ANALYSIS
        long int us = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);
        printf("Time Taken: %ld microseconds\n", us);
    #endif
    
    #ifdef ANALYSIS
    // Check error
    uint ** dp = (uint **) malloc((n-1) * sizeof(uint *));
    for (uint i = 0; i < n-1; i++){
        dp[i] = (uint *) malloc((1 << (n-1)) * sizeof(uint));
    }
    int verify = verify_tsp(n, A, dp, tour);
    if (!verify){
        printf("FAILED\n");
    }else{
        printf("PASSED\n");
    }
    #endif

    // write output
    #ifdef DEBUG
        fprintf(output_file, "ACTUAL OUTPUT\n");
    #endif

    for (int i = 0; i < n; i++){
        fprintf(output_file, "%u%c", tour[i], (i == n-1)? '\n': ' ');
    }
    fprintf(output_file, "%u\n", answer);
    
    // exiting
    fclose(input_file);
    fclose(output_file);
    for (int i = 0; i < n; i++){
        free(A[i]);
        #ifdef ANALYSIS
            if (i != n-1)free(dp[i]);
        #endif
    }
    free(A);
    free(tour);
    #ifdef ANALYSIS
        free(dp);
    #endif
    }
    return 0;
}