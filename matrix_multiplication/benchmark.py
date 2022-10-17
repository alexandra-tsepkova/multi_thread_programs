import subprocess
from matplotlib import pyplot as plt
import time


def make_time_from_size_graph(program, name):
    times = list()
    size_list = [64 * i for i in range(1, 18, 1)]
    for size in size_list:
        start = time.time()
        subprocess.run([f"./{program}", str(size)])
        end = time.time()
        times.append(float(end - start))

    plt.xlabel("matrix size")
    plt.ylabel("time, s")
    plt.grid()
    plt.scatter(size_list, times)
    plt.plot(size_list, times)
    plt.title(f"$T(size)$ for {name}")


def make_time_from_num_threads_graph(program, name):
    times = list()
    size = 2048
    thread_num_list = [2 + i for i in range(0, 11, 2)]
    for n in thread_num_list:
        start = time.time()
        subprocess.run([f"./{program}", str(size), str(n)])
        end = time.time()
        times.append(float(end - start))

    plt.xlabel("thread number")
    plt.ylabel("time, s")
    plt.grid()
    plt.scatter(thread_num_list, times)
    plt.plot(thread_num_list, times)
    plt.title(f"$T(threads)$ for {name}")


subprocess.run(["make"])
plt.figure(figsize=[24, 12])
plt.subplot(221)
make_time_from_size_graph("naive_multiplication", "naive multiplication")

plt.subplot(222)
make_time_from_size_graph("block_multiplication_one_thread", "block multiplication by one thread")

plt.subplot(223)
make_time_from_num_threads_graph("non_block_multiplication_multiple_threads",
                                 "non block multiplication by multiple threads")

plt.subplot(224)
make_time_from_num_threads_graph("block_multiplication_multiple_threads",
                                 "block multiplication by multiple threads")
plt.tight_layout(pad=6, w_pad=6)
plt.show()





