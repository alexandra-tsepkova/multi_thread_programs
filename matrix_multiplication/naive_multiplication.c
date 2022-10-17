#include <stdio.h>
#include <stdlib.h>
#include "methods.h"

int main(int argc, char ** argv){
    if (argc != 2){
        printf("Not enough arguments: enter size of matrix and number of threads\n");
        return -1;
    }
    int N = strtol(argv[1], NULL, 10);
    if (N < 2){
        printf("Wrong matrix size!\n");
        return -1;
    }

    int** matrix_first = create_and_set_matrix(N, 1);
    int** matrix_second = create_and_set_matrix(N, 1);
    int** result_naive = multiplication_naive(matrix_first, matrix_second, N);

    free(result_naive);
    return 0;
}