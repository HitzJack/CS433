#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
#include <assert.h>

struct timeval tv0, tv1;
struct timezone tz0, tz1;

#define N 1e6

int main(int argc, char * argv[]){
    if (argc != 2){
        printf("Please pass number of threads in input\n");
        return 0;
    }
    int t = atoi(argv[1]);
    int i;
    gettimeofday(&tv0, &tz0);
    #pragma omp parallel num_threads (t) private (i)
    {
        for (i=0; i<N; i++) {
            #pragma omp barrier
        }
    }
    gettimeofday(&tv1, &tz1);
    long int time_taken = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);
    printf("Time Taken: %ld microseconds\n", time_taken);
    return 0;
}