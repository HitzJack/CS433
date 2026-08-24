%%cuda
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <cuda_runtime.h>

#define TOL 1e-5
#define ITER_LIMIT 3000

int n;
__managed__ float diff = 0.0;

__global__ void ComputeKernel(float *A, float *A_copy, int n, int span) {

    int id = blockIdx.x * blockDim.x + threadIdx.x;
    for (int i = span*id; i < span*(id+1);i++ ){
        int row = i / (n+2);
        int col = i % (n+2);

        if (row > 0 && row < n+1 && col > 0 && col < n+1) {
            A_copy[row * (n+2) + col] = 0.2 * (A[row * (n+2) + col] + A[row * (n+2) + col - 1] +
                                          A[row * (n+2) + col + 1] + A[(row + 1) * (n+2) + col] +
                                          A[(row - 1) * (n+2) + col]);
        }

    }

    // if (i < n+1 && j < n+1) {
    //     A_copy[i * (n+2) + j] = 0.2 * (A[i * (n+2) + j] + A[i * (n+2) + j - 1] +
    //                                    A[i * (n+2) + j + 1] + A[(i + 1) * (n+2) + j] +
    //                                    A[(i - 1) * (n+2) + j]);
    // }
}

__global__ void UpdateAndDiffKernel(float *A, float *A_copy, int n, int span) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    float local_diff = 0.0;
    for (int i = span*id; i < span*(id+1);i++ ){
        local_diff += fabsf(A[i] - A_copy[i]);
        A[i] = A_copy[i];
    }
    atomicAdd(&diff, local_diff);
}

void Initialize(float *X, int n) {
    for (int i = 0; i < (n+2) * (n+2); i++) {
        if (i % (n+2) == 0 || i % (n+2) == n+1 || i / (n+2) == 0 || i / (n+2) == n+1) X[i] = 0.0;
        else X[i] = ((float)(random() % 100) / 100.0);
        //else X[i] = i & 1? 1.0: 0.5;
    }
}

int main(int argc, char **argv) {
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;

    // if (argc != 3) {
    //     printf("Need grid size (n) and total thread count (t).\nAborting...\n");
    //     exit(1);
    // }

    // n = atoi(argv[1]);
    // int t = atoi(argv[2]);
    n = 1024;
    int t = 16384;

    // Determine number of blocks based on the fixed 8 threads per block
    int numBlocks = t / 8;

    // Set block and grid dimensions
    dim3 threads(2, 4);
    int gridX = sqrt(numBlocks);
    int gridY = (numBlocks + gridX - 1) / gridX;
    dim3 grid(gridX, gridY);

    // Allocate managed memory for A, A_copy, and d_diff
    float *A, *A_copy;
    cudaMallocManaged(&A, (n+2) * (n+2) * sizeof(float));
    cudaMallocManaged(&A_copy, (n+2) * (n+2) * sizeof(float));

    Initialize(A, n);
    Initialize(A_copy, n);
    int iters = 0;
    bool done = false;

    gettimeofday(&tv0, &tz0);

    while (!done) {
        diff = 0.0;

        // Step 1: ComputeKernel fills A_copy with updated values
        ComputeKernel<<<t/8, 8>>>(A, A_copy, n, (n+2)*(n+2) /t);
        cudaDeviceSynchronize();

        // Step 2: UpdateAndDiffKernel loads values back into A and calculates diff
        UpdateAndDiffKernel<<<t/8, 8>>>(A, A_copy, n, (n+2)*(n+2) /t);
        cudaDeviceSynchronize();

        // Calculate average difference for convergence
        diff = diff / (n * n);
        iters++;

        printf("[%d] diff = %.10f\n", iters, diff);

        // Check for convergence or iteration limit
        if (diff < TOL || iters == ITER_LIMIT) done = true;
    }
    gettimeofday(&tv1, &tz1);

    // Free allocated memory
    cudaFree(A);
    cudaFree(A_copy);

    printf("Time: %ld microseconds\n", (tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec));

    return 0;
}
