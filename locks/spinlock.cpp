#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>

int data = 0;


struct Spinlock {
    std::atomic_bool is_locked = false;

    void lock() {
        while (true) {
            if (!is_locked.exchange(true), std::memory_order_acquire) {
                return;
            }
        }
    }

    void unlock() {
        is_locked.store(false, std::memory_order_release);
    }
};


struct TtasSpinlock {
    std::atomic_bool is_locked = false;

    void lock() {
        while (true) {
            if(!is_locked.exchange(true), std::memory_order_acquire){
                return;
            }
            while (is_locked.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }

        }
    }

    void unlock() {
        is_locked.store(false, std::memory_order_release);
    }
};

const int N = 100000;

Spinlock spinlock;
double averages[256];
double maximums[256];

void spinlock_routine(int index) {
    double sum = 0.0;
    double max = 0.0;
    for (int i = 0; i < N; ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        spinlock.lock();
        data = data + 1;
        spinlock.unlock();

        auto end = std::chrono::high_resolution_clock::now();
        double diff = (end - start).count();

        sum += diff;
        max = diff > max ? diff : max;
    }
    averages[index] = sum / N;
    maximums[index] = max;
}

TtasSpinlock tas_spinlock;

void ttas_spinlock_routine(int index) {
    double sum = 0.0;
    double max = 0.0;
    for (int i = 0; i < N; ++i) {

        auto start = std::chrono::high_resolution_clock::now();

        tas_spinlock.lock();
        data = data + 1;
        tas_spinlock.unlock();

        auto end = std::chrono::high_resolution_clock::now();

        double diff = (end - start).count();


        sum += diff;
        max = diff > max ? diff : max;
    }
    averages[index] = sum / N;
    maximums[index] = max;
}

int main() {
    std::printf("%10s | %10s | %10s | %22s | %22s\n", "Threads", "Spin avg", "TTAS avg", "Spin max", "TTAS max");

    double spin_avg, ttas_avg, spin_max, ttas_max;

    int num_threads[9] = {2, 3, 4, 5, 6, 7, 8};
    std::thread *threads[256];
    for (int i = 0; i < 7; ++i) {
        spin_avg = ttas_avg = spin_max = ttas_max = 0.0;
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j] = new std::thread(spinlock_routine, j);
        }
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j]->join();
            delete(threads[j]);

            spin_avg += averages[j];
            spin_max = maximums[j] > spin_max ? maximums[j] : spin_max;
        }
        spin_avg /= num_threads[i];

        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j] = new std::thread(ttas_spinlock_routine, j);
        }

        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j] = new std::thread(ttas_spinlock_routine, j);
        }
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j]->join();
            delete(threads[j]);

            ttas_avg += averages[j];
            ttas_max = maximums[j] > ttas_max ? maximums[j] : ttas_max;
        }
        ttas_avg /= num_threads[i];
        std::printf(
                "%10d | %10llu | %10llu | %22llu | %22llu\n",
                num_threads[i],
                (unsigned long long) spin_avg,
                (unsigned long long) ttas_avg,
                (unsigned long long) spin_max,
                (unsigned long long) ttas_max
        );
    }
}
