#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>
#include <omp.h>

struct timeval tv0, tv1;
struct timezone tz0, tz1;

#define N 16384
int x = 0, y = 0;

void initialize(float * a){
    int i;
    for (i = 0; i < N; i++){
        a[i] = 1.0;
    }
}

int main(int argc, char * argv[]){
    if (argc != 2){
        printf("Please pass number of threads in input\n");
        return 0;
    }
    int t = atoi(argv[1]);
    int i;
    float ** a = (float **)malloc(N * sizeof(float*));
    for (i = 0; i < N; i++){
        a[i] = (float *)malloc(N * sizeof(float));
        initialize(a[i]);
    }
    float * x = (float *)malloc(N * sizeof(float));
    initialize(x);
    float * y = (float *)malloc(N * sizeof(float));
    printf("Size of a: %ld\n", N*N*sizeof(float));
    gettimeofday(&tv0, &tz0);
    #pragma omp parallel for num_threads(t)
    for (i = 0; i < N; i++){
        int j;
        y[i] = 0;
        for (j = 0; j < N; j++){
            y[i] += a[i][j] * x[j];
        }
    } 
    gettimeofday(&tv1, &tz1);
    float sum = 0;
    for (i = 0; i < N; i++){
        sum += y[i];
    }
    printf("Sum: %f\n", sum);
    long int time_taken = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);
    printf("Time Taken: %ld microseconds\n", time_taken);
    return 0;
}