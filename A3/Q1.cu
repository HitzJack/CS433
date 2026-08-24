#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <cuda.h>

#define TOL 1e-5
#define ITER_LIMIT 1000

int n;
__managed__ float diff = 0.0;

__global__ void SolveSharedMem(float *A, int n, int span) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    extern __shared__ float shared_A[];

    float local_diff = 0.0;

    for (int i = span * id; i < span * (id + 1); i++) {
        int row = i / (n + 2);
        int col = i % (n + 2);

        if (row > 0 && row < n + 1 && col > 0 && col < n + 1) {
            int idx = row * (n + 2) + col;
            float temp = shared_A[idx];
            shared_A[idx] = (shared_A[idx] + shared_A[row * (n + 2) + col - 1] +
                             shared_A[row * (n + 2) + col + 1] +
                             shared_A[(row + 1) * (n + 2) + col] +
                             shared_A[(row - 1) * (n + 2) + col]) * 0.2;
            local_diff += fabsf(temp - shared_A[idx]);
        }
    }

    atomicAdd(&diff, local_diff);
}

__global__ void SolveTreeReduction(float *A, int n, int span) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    float local_diff = 0.0;

    for (int i = span * id; i < span * (id + 1); i++) {
        int row = i / (n + 2);
        int col = i % (n + 2);

        if (row > 0 && row < n + 1 && col > 0 && col < n + 1) {
            int idx = row * (n + 2) + col;
            float temp = A[idx];
            A[idx] = (A[idx] + A[row * (n + 2) + col - 1] +
                      A[row * (n + 2) + col + 1] +
                      A[(row + 1) * (n + 2) + col] +
                      A[(row - 1) * (n + 2) + col]) * 0.2;
            local_diff += fabsf(temp - A[idx]);
        }
    }

    __shared__ float shared_diff[256]; // Assuming max 256 threads per block

    shared_diff[threadIdx.x] = local_diff;
    __syncthreads();

    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (threadIdx.x < s) {
            shared_diff[threadIdx.x] += shared_diff[threadIdx.x + s];
        }
        __syncthreads();
    }

    if (threadIdx.x == 0) {
        atomicAdd(&diff, shared_diff[0]);
    }
}

__global__ void Solve(float *A, int n, int span) {

    int id = blockIdx.x * blockDim.x + threadIdx.x;
    float local_diff = 0.0;
    for (int i = span*id; i < span*(id+1);i++ ){
        int row = i / (n+2);
        int col = i % (n+2);

        if (row > 0 && row < n+1 && col > 0 && col < n+1) {
            float temp = A[row * (n+2) + col];
            A[row * (n+2) + col] = 0.2 * (A[row * (n+2) + col] + A[row * (n+2) + col - 1] +
                                          A[row * (n+2) + col + 1] + A[(row + 1) * (n+2) + col] +
                                          A[(row - 1) * (n+2) + col]);
            local_diff += fabsf(temp - A[row * (n+2) + col]);
        }
    }
    atomicAdd(&diff, local_diff);
}


void init_kernel(float *X, int n) {
    for (int i = 0; i < (n+2) * (n+2); i++) {
        X[i] = ((float)(random() % 100) / 100.0);
    }
}

int main(int argc, char **argv) {
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;

    if (argc != 3) {
        printf("Need grid size (n) and total thread count (t).\nAborting...\n");
        exit(1);
    }

    n = argv[1];
    int t = argv[2];

    int numBlocks = t / 8;

    float *A;
    cudaMallocManaged(&A, (n+2) * (n+2) * sizeof(float));

    init_kernel(A, n);
    int iters = 0;
    bool done = false;

    gettimeofday(&tv0, &tz0);

    while (!done) {
        diff = 0.0;

        Solve<<<t/8, 8>>>(A, n, (n+2)*(n+2) /t);
        //    SolveTreeReduction<<<t/8, 8>>>(A, n, (n+2)*(n+2) /t);
        //    SolveSharedMem<<<t / 8, 8, (n + 2) * (n + 2) * sizeof(float)>>>(A, n, (n+2)*(n+2) / t);
        cudaDeviceSynchronize();

        diff = diff / (n * n);
        iters++;

         // printf("[%d] diff = %.10f\n", iters, diff);

        if (diff < TOL || iters == ITER_LIMIT) done = true;
    }
    gettimeofday(&tv1, &tz1);

    cudaFree(A);

    printf("Time: %ld microseconds\n", (tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec));

    return 0;
}
