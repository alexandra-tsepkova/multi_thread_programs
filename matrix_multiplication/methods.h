int** create_and_set_matrix(int size, int fill);

void print_matrix(int** matrix, int size);

int** multiplication_naive(int** matrix_first, int** matrix_second, int size);

int** multiplication_block_one_thread(int** matrix_first, int** matrix_second, int size);

int** multiplication_not_block_multiple_threads(int** matrix_first, int** matrix_second, int size, int thread_count);

int** multiplication_block_multiple_threads(int** matrix_first, int** matrix_second, int size, int thread_count);

void* thread_job_not_block(void* thread_args);

void* thread_job_block(void* thread_args);