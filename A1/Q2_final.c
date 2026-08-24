#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
#define ANALYSIS
#define PRECISION 1000
#define double long double
typedef struct padded_sum{
    double val;
    char padding[48];
} padded_sum;

#ifdef ANALYSIS
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;
#endif


void algo_sequential(int n, int num_threads, double ** A, double * b, double * x){
    double subs_total;
    for (int i = 0; i < n; i++){
        subs_total = 0.0;
        for (int j = 0; j < i; j++){
            subs_total += x[j] * A[i][j];
        }
        x[i] = (b[i] - subs_total) / A[i][i];
    }
}
void solve(int n, int num_threads, double ** A, double * b, double * x){
    double row_total = 0.0;
    padded_sum private_total[num_threads];
    int done_upto = 0, i, tid, var;
    for (int j = 0; j < num_threads; j++){
        private_total[j].val = 0.0;
    }

    #ifdef ANALYSIS
        gettimeofday(&tv0, &tz0);
    #endif

    // if (num_threads == 1){ // sequential code
    //     algo_sequential(n, num_threads, A, b, x);
    //     #ifdef ANALYSIS
    //     // Stop Timer
    //         gettimeofday(&tv1, &tz1);
    //     #endif

    //     return;
    // }
    // -- Parallel code --

    #pragma omp parallel num_threads(num_threads) private(var, tid, i)
    for (var = 0; var < n; var++){
        tid = omp_get_thread_num();
        #pragma omp for
        for (i = 0; i < var; i++){
            private_total[tid].val += A[var][i] * x[i];
        }
        #pragma omp single
        {
            row_total = 0.0;
            for (int j = 0; j < num_threads; j++){
                row_total += private_total[j].val;
                private_total[j].val = 0.0;
            }
            x[var] = (b[var] - row_total) / A[var][var];
        }
    }

    #ifdef ANALYSIS
    // Stop Timer
        gettimeofday(&tv1, &tz1);
    #endif

    return;
}

int main(int argc, char *argv[]){
    if (argc != 4){
        perror("Need 3 arguments: input_filename output_filename num_threads\n");
        return -1;
    }
    // Read arguments
    int num_threads;
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
    int n;
    int temp;
    temp = fscanf(input_file, "%d", &n);
    if (num_threads > n){
        num_threads = n;
    }
    double ** A = (double **) malloc(n * sizeof(double *));
    double * x = (double *) malloc(n * sizeof(double));
    double * b = (double *) malloc(n * sizeof(double));

    /* INITIALIZATION */
    #ifdef ANALYSIS
        double * x_answer = (double *) malloc(n * sizeof(double));
        for (int i = 0; i < n; i++){
            x_answer[i] = (2*(rand() % 2) - 1) * ((rand() % PRECISION) / (1.0 * PRECISION) + 1.0);
            x[i] = 0.0;
        }

        for (int i = 0; i < n; i++){
            A[i] = (double *) malloc((i+1) * sizeof(double));
            b[i] = 0.000;
            for (int j = 0; j <= i; j++){
                A[i][j] = (2*(rand() % 2) - 1) * ((rand() % PRECISION) / (1.0 * PRECISION) + 1.0);
                b[i] += x_answer[j]*A[i][j];
            }
        }
    #else
        for (int i = 0; i < n; i++){
            A[i] = (double *) malloc((i+1) * sizeof(double));
            for (int j = 0; j <= i; j++){
                temp = fscanf(input_file, "%Lf", &A[i][j]);            
            }
        }
        for (int i = 0; i < n; i++){
            temp = fscanf(input_file, "%Lf", &b[i]); 
            x[i] = 0.0;
        }
    #endif

    solve(n, num_threads, A, b, x);

    #ifdef ANALYSIS
        long int us = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);
        printf("Time Taken: %ld microseconds\n", us);
    #endif
    
    #ifdef ANALYSIS
    // Check error
        double err = 0.0, curr = 0.0, eps = 0.1;
        for (int i = 0; i < n; i++){
            curr = (x_answer[i] - x[i]);
            if (curr < 0.0){
                curr = -curr;
            }
            if (curr > err){
                err = curr;
            }
            if (curr > eps){
                printf("INCORRECT!! Huge error (%Lf) at index %d, expected %Lf, output %Lf\n", curr, i+1, x_answer[i], x[i]);
                break;
            }
        }
        printf("The max absolute error at any index is %Lf\n", err);    
    #endif

    // write output
    #ifdef DEBUG
        fprintf(output_file, "ACTUAL OUTPUT\n");
    #endif

    for (int i = 0; i < n; i++){
        fprintf(output_file, "%Lf%c", x[i], (i == n-1)? '\n': ' ');
    }

    // exiting
    fclose(input_file);
    fclose(output_file);
    for (int i = 0; i < n; i++){
        free(A[i]);
    }
    free(A);
    free(x);
    free(b);
    #ifdef ANALYSIS
        free(x_answer);
    #endif

    return 0;
}