#include "methods.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define block_size 32

struct thread_args{  // args for non-block multiplication by multiple threads
    int begin;
    int num;
    int size;
    int** matrix_first;
    int** matrix_second;
    int** result;
};


int** create_and_set_matrix(int size, int fill){
    int** matrix = (int **)malloc(size * sizeof(int *));

    for (int i = 0; i < size; i++)
        matrix[i] = (int *) malloc(size * sizeof(int));

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            matrix[i][j] = fill;
        }
    }
    return matrix;
}

void print_matrix(int** matrix, int size){
    for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
            printf("%d  ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int** multiplication_naive(int** matrix_first, int** matrix_second, int size){
    int** result = create_and_set_matrix(size, 0);

    for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
            for (int k = 0; k < size; k++){
                result[i][j] += matrix_first[i][k] * matrix_second[k][j];
            }
        }
    }
    return result;
}

int** multiplication_block_one_thread(int** matrix_first, int** matrix_second, int size){
    int** result = create_and_set_matrix(size, 0);

    int block_first_matrix[block_size][block_size];
    int block_second_matrix[block_size][block_size];
    int block_res[block_size][block_size];
    int num_blocks = size / block_size;

    for (int i_res = 0; i_res < size; i_res += block_size){
        for (int j_res = 0; j_res < size; j_res += block_size){

            // initialize block_res with zeros
            for (int c = 0; c < block_size; c++){
                for (int d = 0; d < block_size; d++){
                    block_res[c][d] = 0;
                }
            }

            // iterate over blocks to count block_res
            for (int count_blocks = 0; count_blocks < num_blocks; count_blocks++){
                // set block_first_matrix and block_second_matrix
                for (int i = 0; i < block_size; i++){
                    for(int j = 0; j < block_size; j++){
                        block_first_matrix[i][j] = matrix_first[i_res + i][count_blocks * block_size + j];
                        block_second_matrix[i][j] = matrix_second[count_blocks * block_size + i][j_res + j];
                    }
                }

                // multiply blocks
                for (int i = 0; i < block_size; i++){
                    for(int j = 0; j < block_size; j++){
                        for(int k = 0; k < block_size; k++){
                            block_res[i][j] +=  block_first_matrix[i][k] * block_second_matrix[k][j];
                        }
                    }
                }
            }

            // add counted block to result_matrix
            for(int i = 0; i < block_size; i++){
                for(int j = 0; j < block_size; j++){
                    result[i_res + i][j_res + j] = block_res[i][j];
                }
            }
        }
    }
    return result;
}

void* thread_job_not_block(void* thread_args){
    struct thread_args* thread_args_new = (struct thread_args *)thread_args;
    for (int i = thread_args_new->begin; i < thread_args_new->begin
    + thread_args_new->num; i++){
        for (int j = 0; j < thread_args_new->size; j++){
            for (int k = 0; k < thread_args_new->size; k++){
                thread_args_new->result[i][j] += thread_args_new->matrix_first[i][k] * thread_args_new->matrix_second[k][j];
            }
        }
    }
}

int** multiplication_not_block_multiple_threads(int** matrix_first, int** matrix_second, int size, int thread_count){
    int** result = create_and_set_matrix(size, 0);
    pthread_t *threads;
    threads = malloc(thread_count * sizeof(pthread_t));
    struct thread_args args[thread_count];

    int num_lines_all_but_first = size / thread_count;
    int num_lines_first = num_lines_all_but_first + size % thread_count;

    for (int i = 0; i < thread_count; i++) {
        args[i].matrix_first = matrix_first;
        args[i].matrix_second = matrix_second;
        args[i].result = result;
        args[i].size = size;
        if (i == 0){
            args[i].begin= 0;
            args[i].num = num_lines_first;
        }
        else{
            args[i].begin= num_lines_first + (i - 1) * num_lines_all_but_first;
            args[i].num = num_lines_all_but_first;
        }
        int ret = pthread_create(&threads[i], NULL, thread_job_not_block, (void* ) &args[i]);
        if (ret != 0){
            printf("Error creating thread!\n");
            break;
        }
    }
    for(int i = 0; i < thread_count; i++){
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return result;
}

void* thread_job_block(void* thread_args){
    struct thread_args* thread_args_new = (struct thread_args *)thread_args;
    int block_first_matrix[block_size][block_size];
    int block_second_matrix[block_size][block_size];
    int block_res[block_size][block_size];

    for (int i_res = thread_args_new->begin; i_res < thread_args_new->begin + thread_args_new->num * block_size; i_res += block_size){
        for (int j_res = 0; j_res < thread_args_new->size; j_res += block_size){

            // initialize block_res with zeros
            for (int c = 0; c < block_size; c++){
                for (int d = 0; d < block_size; d++){
                    block_res[c][d] = 0;
                }
            }

            // iterate over blocks to count block_res
            for (int count_blocks = 0; count_blocks < thread_args_new->size / block_size;  count_blocks++){
                // set block_first_matrix and block_second_matrix
                for (int i = 0; i < block_size; i++){
                    for(int j = 0; j < block_size; j++){
                        block_first_matrix[i][j] = thread_args_new->matrix_first[i_res + i][count_blocks * block_size + j];
                        block_second_matrix[i][j] = thread_args_new->matrix_second[count_blocks * block_size + i][j_res + j];
                    }
                }

                // multiply blocks
                for (int i = 0; i < block_size; i++){
                    for(int j = 0; j < block_size; j++){
                        for(int k = 0; k < block_size; k++){
                            block_res[i][j] +=  block_first_matrix[i][k] * block_second_matrix[k][j];
                        }
                    }
                }
            }

            // add counted block to result_matrix
            for(int i = 0; i < block_size; i++){
                for(int j = 0; j < block_size; j++){
                    thread_args_new->result[i_res + i][j_res + j] = block_res[i][j];
                }
            }
        }
    }
}

int** multiplication_block_multiple_threads(int** matrix_first, int** matrix_second, int size, int thread_count){
    int** result = create_and_set_matrix(size, 0);
    pthread_t *threads;
    threads = malloc(thread_count * sizeof(pthread_t));
    struct thread_args args[thread_count];
    int total_num_blocks = size / block_size;
    int num_blocks_all_but_first = total_num_blocks / thread_count;
    int num_blocks_first = num_blocks_all_but_first + total_num_blocks % thread_count;

    for (int i = 0; i < thread_count; i++) {
        args[i].matrix_first = matrix_first;
        args[i].matrix_second = matrix_second;
        args[i].result = result;
        args[i].size = size;
        if (i == 0){
            args[i].begin= 0;
            args[i].num = num_blocks_first;
        }
        else{
            args[i].begin= (num_blocks_first + (i - 1) * num_blocks_all_but_first) * block_size;
            args[i].num = num_blocks_all_but_first;
        }
        int ret = pthread_create(&threads[i], NULL, thread_job_block, (void* ) &args[i]);
        if (ret != 0){
            printf("Error creating thread!\n");
            break;
        }
    }
    for(int i = 0; i < thread_count; i++){
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return result;
}




