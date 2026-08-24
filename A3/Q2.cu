#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>
#include <math.h>
#include <sys/time.h>

#define TILE_SIZE 16

__global__ void init_kernel(float* A, int n, int span) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    for (int i = span * id; i < span * (id + 1); i++) {
        A[i] = (float)(i ^ n) / (n);
        A[i] = (A[i] + 1) / 2; // Normalize to [0.5, 1] to avoid single precision issues
    }
}

__global__ void Solver(float* A, float* x, float* y, int n, int span) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    int l = span * id;
    int r = span * (id + 1);
    int rowIdx;
    for (int i = l; i < r; i++) {
        rowIdx = i * n;
        y[i] = 0.0f;
        for (int j = 0; j < n; j++) {
            y[i] += A[rowIdx + j] * x[j];
        }
    }
}

// Shared memory optimized Solver Kernel for 16384 threads
__global__ void SolverSharedMem(float* A, float* x, float* y, int n) {
    extern __shared__ float shared_x[];
    int id = blockIdx.x * blockDim.x + threadIdx.x;

    if (id < n) {
        float sum = 0.0f;

        for (int tile = 0; tile < n / TILE_SIZE; ++tile) {
            // Load a tile of x into shared memory
            if (tile * TILE_SIZE + threadIdx.x < n) {
                shared_x[threadIdx.x] = x[tile * TILE_SIZE + threadIdx.x];
            } else {
                shared_x[threadIdx.x] = 0.0f;
            }
            __syncthreads();

            for (int j = 0; j < TILE_SIZE && (tile * TILE_SIZE + j) < n; ++j) {
                sum += A[id * n + tile * TILE_SIZE + j] * shared_x[j];
            }
            __syncthreads();
        }

        y[id] = sum;
    }
}

void SolverCPU(float* A, float* x, float* y, int n) {
    for (int i = 0; i < n; i++) {
        y[i] = 0.0f;
        int t = i * n;
        for (int j = 0; j < n; j++) {
            y[i] += A[t + j] * x[j];
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3){
        printf("Need n and t\n");
        return 0;
    }
    int n = argv[1];
    int nthreads = argv[2];

    struct timeval tv0, tv1;
    struct timezone tz0, tz1;

    float *A, *x, *y_cpu, *y_gpu;

    cudaMallocManaged(&x, n * sizeof(float));
    cudaMallocManaged(&A, n * n * sizeof(float));
    cudaMallocManaged(&y_gpu, n * sizeof(float));
    cudaMallocManaged(&y_cpu, n * sizeof(float));

    int blocks = (nthreads < 16) ? 1 : nthreads / 8;
    int threads = (nthreads < 16) ? nthreads : 8;

    init_kernel<<<blocks, threads>>>(A, n*n, (n*n) / nthreads);
    init_kernel<<<blocks, threads>>>(x, n, n / nthreads);
    // Initializevector<<<blocks, threads>>>(x, n, n / nthreads);
    cudaDeviceSynchronize();

    gettimeofday(&tv0, &tz0);

    // Uncomment the original Solver kernel call to use it instead of the optimized one
    if (nthreads < 16) {
        Solver<<<1, nthreads>>>(A, x, y_gpu, n, n / nthreads);
        cudaDeviceSynchronize();
    }
    else {
        Solver<<<nthreads / 8, 8>>>(A, x, y_gpu, n, n / nthreads);
        cudaDeviceSynchronize();
    }

    gettimeofday(&tv1, &tz1);

    SolverCPU(A, x, y_cpu, n);

    float avg_diff = 0.0f;
    for (int i = 0; i < n; i++) {
        avg_diff += fabs(y_cpu[i] - y_gpu[i]);
    }
    avg_diff /= n;

    printf("Num threads: %d,Average Difference: %f, Time: %ld microseconds\n", nthreads, avg_diff, (tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec));

    cudaFree(A);
    cudaFree(x);
    cudaFree(y_gpu);
    cudaFree(y_cpu);

/*
    // Shared memory optimized Solver Kernel for 16384 threads
    int bestthreadcount = 16384;
    int nthreads = bestthreadcount;

    struct timeval tv0, tv1;
    struct timezone tz0, tz1;

    float *A, *x, *y_cpu, *y_gpu;

    cudaMallocManaged(&A, n * n * sizeof(float));
    cudaMallocManaged(&x, n * sizeof(float));
    cudaMallocManaged(&y_cpu, n * sizeof(float));
    cudaMallocManaged(&y_gpu, n * sizeof(float));

    int blocks = (nthreads < 16) ? 1 : nthreads / 8;
    int threads = (nthreads < 16) ? nthreads : 8;

    init_kernel<<<blocks, threads>>>(A, n*n, (n * n) / nthreads);
    init_kernel<<<blocks, threads>>>(x, n, n / nthreads);
    cudaDeviceSynchronize();


    gettimeofday(&tv0, &tz0);
    SolverSharedMem<<<nthreads/TILE_SIZE, TILE_SIZE, TILE_SIZE * sizeof(float)>>>(A, x, y_gpu, n);
    cudaDeviceSynchronize();
    gettimeofday(&tv1, &tz1);

    SolverCPU(A, x, y_cpu, n);


    // Calculate average difference
    float avg_diff = 0.0f;
    for (int i = 0; i < n; i++) {
        avg_diff += fabs(y_cpu[i] - y_gpu[i]);
    }
    avg_diff /= n;

    printf("For shared memory\n");
    printf("Number of threads = %d ", nthreads);
    printf("Average Difference: %f, Time: %ld microseconds\n", avg_diff, (tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec));

    // Free allocated memory
    cudaFree(A);
    cudaFree(x);
    cudaFree(y_gpu);
    cudaFree(y_cpu);
*/
    return 0;
}
