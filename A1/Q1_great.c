#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
// Toggle between analysis and submission mode, submission mode follows submission guidelines
// analysis mode prints the time and randomly allocates the graph edge weights (just reads n from input file)
#define ANALYSIS

#define inf 1e9
typedef unsigned int uint;
typedef __uint128_t lluint;

#ifdef ANALYSIS
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;
#endif

// generate bitmasks generates bitmasks and stores them in the provided array such that bitmasks are sorted by the number of set bits
// 32 bit and 128 bit variant just differ in the type used
void generate_bitmasks_32(uint * arr, uint * llimit, uint * rlimit, uint n){
    uint l = (1 << n) - 1;
    arr[0] = 0;
    llimit[1] = 1;
    uint t1 = 0, t2, nb = 1, curr = 1;
    for (uint i = 1; i < l; i++){
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
void generate_bitmasks_128(lluint * arr, lluint * llimit, lluint * rlimit, int n){
    lluint l = (1LL << n) - 1;
    arr[0] = 0;
    llimit[1] = 1;
    lluint t1 = 0, t2, nb = 1, curr = 1;
    for (lluint i = 1; i < l; i++){
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

// Algorithm for solving TSP (32-bit variant, more details about algo in report)
uint solve_32(int n, int num_threads, uint ** A, uint * tour){
    const uint lm = (1 << (n-1));
    uint * masks = (uint *) malloc( lm * sizeof(uint));
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
    
    generate_bitmasks_32(masks, llimit, rlimit, n-1);
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

    free(masks);
    for (int i = 0; i < lm; i++){
        free(dp[i]);
        free(prev[i]);
    }
    free(dp);
    free(prev);

    return mn;
}
// Algorithm for solving TSP (128-bit variant (won't be used if n < 32 which is the most practically testable case), more details about algo in report)
uint solve_128(int n, int num_threads, uint ** A, uint * tour){
    const lluint lm = (1 << (n-1));
    lluint * masks = (lluint *) malloc( lm * sizeof(lluint));
    lluint llimit[n];
    lluint rlimit[n];
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
    generate_bitmasks_128(masks, llimit, rlimit, n-1);
    lluint i, j, k, l, tj, tk, tl;
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
        if (dp[(lm - 1) ^ (1LL<<i)][i] + A[i][n-1] < mn){
            mn = dp[(lm - 1) ^ (1LL<<i)][i] + A[i][n-1];
            lastIndex = i;
            temp = (lm - 1) ^ (1LL<<i);
        }
    }
    tour[0] = lastIndex + 1;
    for (uint i = 1; i < n; i++){
        tour[i] = prev[temp][lastIndex] + 1;
        lastIndex = prev[temp][lastIndex];
        temp ^= (1LL << (tour[i]-1));
    }

    #ifdef ANALYSIS
        gettimeofday(&tv1, &tz1);
    #endif

    free(masks);
    for (int i = 0; i < lm; i++){
        free(dp[i]);
        free(prev[i]);
    }
    free(dp);
    free(prev);

    return mn;
}

uint solve(int n, int num_threads, uint ** A, uint * tour){
    if (n < 32){
        return solve_32(n, num_threads, A, tour);
    }
    return solve_128(n, num_threads, A, tour);
}

int main(int argc, char *argv[]){
    if (argc != 4){
        printf("Need 3 arguments: input_filename output_filename num_threads\n");
        return 0;
    }
    // Read arguments
    // int tests = atoi(argv[3]);
    int num_threads;
    FILE * input_file = fopen(argv[1], "r");
    FILE * output_file = fopen(argv[2], "w");
    num_threads = atoi(argv[3]);
    if (input_file == NULL){
        printf("Could not open input file\n");
        return 0;
    }
    if (output_file == NULL){
        printf("Could not open output file\n");
        return 0;
    }
    #ifdef ANALYSIS
        FILE * analysis_file = fopen("analysis.csv", "w");
    #endif 

    int threads[] = {1,2,3,4,6,8};
    // Read input and allocate necessary memory
    // for (int testcase = 0; testcase < tests; testcase++){
        // num_threads = threads[(testcase/8)%6];
    int n;
    int temp;
    temp = fscanf(input_file, "%d", &n);
    if (num_threads > n){
        num_threads = n;
    }
    uint ** A = (uint **) malloc(n * sizeof(uint *));
    uint * tour = (uint *) malloc(n * sizeof(uint));

    /* INITIALIZATION */
    #ifdef DANALYSIS
        for (int i = 0; i < n; i++){
            A[i] = (uint *) malloc((n) * sizeof(uint));
        }
        for (int i = 0; i < n; i++){
            A[i][i] = 0;
            for (int j = i+1; j < n; j++){
                A[i][j] = rand() % 100 + 1;
                A[j][i] = A[i][j];
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

    // Solve
    uint answer = solve(n, num_threads, A, tour);

    #ifdef ANALYSIS
        long int us = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);
        // printf("%d. %d, %d, %ld\n", num_threads, n, testcase / 48 + 1, us);
        printf("Time Taken: %ld microseconds\n", us);
        // fprintf(analysis_file,"%d, %d, %d, %ld\n", num_threads, n, testcase / 48 + 1, us);
    #endif

    // write output
    // for (int i = 0; i < n; i++){
    //     fprintf(output_file, "%u%c", tour[i], (i == n-1)? '\n': ' ');
    // }
    fprintf(output_file, "%u\n", answer);

    // exiting
    for (int i = 0; i < n; i++){
        free(A[i]);
    }
    free(A);
    free(tour);
    // }
    // // fclose(input_file);
    // // fclose(output_file);
    return 0;
}