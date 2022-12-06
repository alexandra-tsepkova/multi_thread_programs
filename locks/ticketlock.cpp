#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>

struct Ticketlock {
    std::atomic_int number = 0;
    std::atomic_int current_number = 0;

    void lock() {
        int ticket = number.fetch_add(1);
        while(current_number.load(std::memory_order_acquire) != ticket);
    }

    void unlock() {
        current_number.store(current_number+1, std::memory_order_release);
    }
};

const int N = 100000;
int data = 1;
Ticketlock ticketlock;
double averages[256];
double maximums[256];

void ticketlock_routine(int index) {
    double sum = 0.0;
    double max = 0.0;
    for (int i = 0; i < N; ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        ticketlock.lock();
        data = data + 1;
        ticketlock.unlock();

        double diff = (std::chrono::high_resolution_clock::now() - start).count();

        sum += diff;
        max = diff > max ? diff : max;
    }
    averages[index] = sum / N;
    maximums[index] = max;
}


int main() {
    std::printf("%10s | %10s |  %22s\n", "Threads", "Avg", "Max");

    double avg, max;

    int num_threads[7] = {2, 3, 4, 5, 6, 7, 8};
    std::thread *threads[256];
    for (int i = 0; i < 7; ++i) {
        avg = max = 0.0;
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j] = new std::thread(ticketlock_routine, j);
        }
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j]->join();
            delete(threads[j]);

            avg += averages[j];
            max = maximums[j] > max ? maximums[j] : max;
        }
        avg /= num_threads[i];

        std::printf(
                "%10d | %10llu | %22llu\n",
                num_threads[i],
                (unsigned long long) avg,
                (unsigned long long) max
        );
    }
}
