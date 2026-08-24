#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#define inf 1e9
typedef unsigned int uint;
uint tsp(uint n, uint ** w, uint ** dp, uint ** prev, uint * tour, uint num_threads){
    uint temp;
    for (uint i =0; i < n-1; i++){
        dp[i][0] = w[n-1][i];
        prev[i][0] = n-1;
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

int main(uint argc, char *argv[]){
    if (argc != 4){
        perror("Need 3 arguments: input_filename output_filename num_threads\n");
        return -1;
    }
    // Read arguments
    uint num_threads;
    FILE * input_file = fopen(argv[1], "r");
    FILE * output_file = fopen(argv[2], "w");
    num_threads = atoi(argv[3]);
    if (input_file == NULL){
        perror("Could not open input file\n");
    }
    if (output_file == NULL){
        perror("Could not open output file\n");
    }

    // Read input and allocate necessary memory
    uint n;
    uint temp;
    temp = fscanf(input_file, "%d", &n);
    uint ** edge_weights = (uint **) malloc(n * sizeof(uint *));
    for (uint i = 0; i < n; i++){
        edge_weights[i] = (uint *) malloc((n) * sizeof(uint));
        edge_weights[i][i] = 0;
    }
    uint x;
    for (uint i = 0; i < n; i++){
        for (uint j = i+1; j < n; j++){
            temp = fscanf(input_file, "%d", &x);
            edge_weights[i][j] = x;
            edge_weights[j][i] = x;
        }
    }
    uint * tour = (uint *) malloc(n * sizeof(uint));

    uint ** dp = (uint **) malloc((n-1) * sizeof(uint *));
    uint ** prev = (uint **) malloc((n-1) * sizeof(uint *));
    for (uint i = 0; i < n-1; i++){
        dp[i] = (uint *) malloc((1 << (n-1)) * sizeof(uint));
        prev[i] = (uint *) malloc((1 << (n-1)) * sizeof(uint));
        for (uint j = 0; j < (1 << (n-1)); j++){
            dp[i][j] = inf;
        }
    }
    // Solve
    uint min_weight = tsp(n, edge_weights, dp, prev, tour, num_threads);

    // write output
    for (uint i = 0; i < n; i++){
        fprintf(output_file, "%d%c", tour[i], (i == n-1)? '\n': ' ');
    }
    fprintf(output_file, "%d\n", min_weight);

    // exiting
    fclose(input_file);
    fclose(output_file);
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

    return 0;
}