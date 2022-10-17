#include <stdio.h>
#include <stdlib.h>
#include "methods.h"

int main(int argc, char ** argv){
    if (argc != 3){
        printf("Not enough arguments: enter size of matrix and number of threads\n");
        return -1;
    }
    int N = strtol(argv[1], NULL, 10);
    int thread_count = strtol(argv[2], NULL, 10);


    if (N < 2){
        printf("Wrong matrix size!\n");
        return -1;
    }

    int** matrix_first = create_and_set_matrix(N, 1);
    int** matrix_second = create_and_set_matrix(N, 1);
    int** result_not_block_mult_thread = multiplication_not_block_multiple_threads(matrix_first, matrix_second, N, thread_count);

    free(result_not_block_mult_thread);
    return 0;
}