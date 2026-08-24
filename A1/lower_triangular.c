#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

#define mat_t double
#define BLACK  "\033[1;30m"
#define RED  "\033[1;31m"
#define GREEN  "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE  "\033[1;34m"
#define MAGENTA  "\033[1;35m"
#define CYAN  "\033[1;36m"
#define WHITE  "\033[1;37m"
#define OFF "\033[0m"



#define NARGS 3

typedef struct{
    size_t n;
    mat_t** l;
    mat_t* y; 
} Input;

void algo_sequential(int n, mat_t ** A, mat_t * b, mat_t * x){
    mat_t subs_total;
    for (int i = 0; i < n; i++){
        subs_total = 0.0;
        for (int j = 0; j < i; j++){
            subs_total += x[j] * A[i][j];
        }
        x[i] = (b[i] - subs_total) / A[i][i];
    }
}


// Input* InitializeInput(char* path){
//     FILE* file_ptr = fopen(path,"r");
//     size_t n;
//     fscanf(file_ptr,"%ld",&n);
//     Input* store_input = calloc(1,sizeof(Input));
//     store_input->n = n;
//     store_input->l =  (mat_t*)calloc(n*(n+1)/2,sizeof(mat_t));
    
//     for(size_t i = 0;i<n*(n+1)/2;i++){
//         fscanf(file_ptr,"%lf",(store_input->l + i));
//     }

//     store_input->y =  (mat_t*)calloc(n,sizeof(mat_t));
//     for(size_t i = 0;i<n;i++){
//         fscanf(file_ptr,"%lf",(store_input->y + i));
//     }
//     return store_input;
// }

int parallel_solver(Input* augmented, mat_t* output,int nthreads){
    
    // printf("%d\n",nthreads);
    double x;
    size_t i,j;
    #pragma omp parallel num_threads (nthreads) private(i,j)
    for(i = 0;i<augmented->n;i++){
        #pragma omp single
        {
            x = ((double)(augmented->y[i]))/((double)augmented->l[i][i]); 
            output[i] = x;
        }
        #pragma omp for
        for(j = i+1;j<augmented->n;j++){
            augmented->y[j] -= x*(augmented->l[j][i]);
        }
        
    }

    // for(int i = 0;i<augmented->n;i++){
    //     printf("%lf ",output[i]);
    // }
    // printf("\n");

}


int sequential_solver(Input* augmented, mat_t* output){
    double x;
    for(size_t i = 0;i<augmented->n;i++){
        x = ((double)(augmented->y[i]))/((double)augmented->l[i][i]); 
        output[i] = x;
        for(size_t j = i+1;j<augmented->n;j++){
            augmented->y[j] -= x*(augmented->l[j][i]);
        }
    }
    // for(int i = 0;i<augmented->n;i++){
    //     printf("%lf ",output[i]);
    // }printf("\n");
}

Input* generate_matrix(int mat_size){
    int n = mat_size;
    mat_t** matrix = (mat_t**)malloc(n*sizeof(mat_t*));
    Input* ret_val = (Input*)malloc(sizeof(Input));

    for(int i = 0;i<n;i++){
        mat_t* array = (mat_t*)malloc((i+1)*sizeof(mat_t));
        for(int j = 0;j<i+1;j++){
            array[j] = (mat_t)(rand()%5 +1);
            // printf("%lf ",array[j]);
        }
        // printf("\n");
        matrix[i] = array;
    }
    
    ret_val->y = (mat_t*)malloc(n*sizeof(mat_t));

    for(int i = 0;i<n;i++){
        ret_val->y[i] = rand()%100;    
        // printf("%lf ",ret_val->y[i]);
    }
    // printf("\n");

    ret_val->n = n;
    ret_val->l = matrix;

    return ret_val;
}

mat_t* copy_array(size_t n,const mat_t* orig){
    int arr_size = sizeof(mat_t)*n;
    mat_t* dupl =(mat_t*) malloc(arr_size);
    // printf("%llx %llx\n",(long long int)dupl,(long long int)orig);

    memcpy(dupl,orig,arr_size);    
    return dupl;
}


int main(int argc, char** argv)
{   

    char path_to_file[100] = "./input.txt" ;
    // Input* stack_store = InitializeInput(path_to_file);
    struct timeval tv0,tv1;
    struct timezone tz0,tz1;
    if(argc < NARGS){
        fprintf(stderr, RED "Arg 1: Nthreads\nArg 2: Matrix Size\n" OFF);
        return -1;
    }
    int nthreads = atoi(argv[1]);
    int mat_size = atoi(argv[2]);
    


    Input* augmented_matrix = generate_matrix(mat_size);
    Input parallel_matrix = *augmented_matrix;
    Input sequential_matrix = *augmented_matrix;
    sequential_matrix.y = copy_array(augmented_matrix->n,augmented_matrix->y);
    // printf("%llx %llx\n",(long long int)sequential_matrix->y,(long long int)parallel_matrix->y);
    
    mat_t * sequential_output = (mat_t*)malloc(sizeof(mat_t)*sequential_matrix.n);
    mat_t * parallel_output = (mat_t*)malloc(sizeof(mat_t)*parallel_matrix.n);
    mat_t * anuj_output = (mat_t*)malloc(sizeof(mat_t)*sequential_matrix.n);
    gettimeofday(&tv0,&tz0);
    sequential_solver(&sequential_matrix,sequential_output);
    gettimeofday(&tv1,&tz1);
    long sequential_time = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);

    gettimeofday(&tv0,&tz0);
    parallel_solver(&parallel_matrix,parallel_output,nthreads);
    gettimeofday(&tv1,&tz1);
    long parallel_time = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);


    gettimeofday(&tv0,&tz0);
    algo_sequential(parallel_matrix.n, sequential_matrix.l, sequential_matrix.y, anuj_output);
    gettimeofday(&tv1,&tz1);
    long anuj_seq_time = (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec);

    int flag = 0;
    double tol = 0.1;
    for(int i = 0;i<augmented_matrix->n;i++){
        printf("%lf %lf\n", parallel_output[i], anuj_output[i]);
        if((parallel_output[i] - anuj_output[i]>tol) ||((anuj_output[i] -parallel_output[i] )>tol) ){
            flag = 1;
            break;
        }
    }

    if(flag ==1){
        printf(RED "PANIC\n" OFF);
    }

	printf("Sequential Time: %ld microseconds\n",sequential_time);    
	printf("Anuj Sequential Time: %ld microseconds\n",anuj_seq_time);    
	printf("Parallel Time: %ld microseconds\n",parallel_time);    

    printf("\n");
    return 0;
}