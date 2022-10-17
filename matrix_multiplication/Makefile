all: naive block_one non_block_mult block_mult

naive: naive_multiplication.c methods.c
	gcc naive_multiplication.c methods.c -o naive_multiplication -pthread
block_one: block_multiplication_one_thread.c methods.c
	gcc block_multiplication_one_thread.c methods.c -o block_multiplication_one_thread -pthread
non_block_mult: non_block_multiplication_multiple_threads.c methods.c
	gcc non_block_multiplication_multiple_threads.c methods.c -o non_block_multiplication_multiple_threads -pthread
block_mult: block_multiplication_multiple_threads.c methods.c
	gcc block_multiplication_multiple_threads.c methods.c -o block_multiplication_multiple_threads -pthread