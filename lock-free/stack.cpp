#include <atomic>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdio>
#include <mutex>

struct LFNode;
struct LFStack;

const int max_threads = 12;
const int HP_per_thread = 1;
thread_local int d_list_top = 0;
const int d_list_max_size = max_threads * HP_per_thread * 2;
LFNode * HP[max_threads * HP_per_thread];
thread_local LFNode *d_list[d_list_max_size];
int current_threads = 0;


struct LFNode{
    int data = 0;
    LFNode *next = nullptr;
};

void free_d_list(){
    LFNode *d_list_new[d_list_max_size];
    int d_list_new_top = 0;
    bool flag;

    for (int i = 0; i < d_list_max_size; i++ ) {
        flag = false;
        for(int j = 0; j < current_threads * HP_per_thread; ++j) {
            if(d_list[i] == HP[j]){
                d_list_new[d_list_new_top] = d_list[i];
                d_list_new_top++;
                flag = true;
                break;
            }
        }
        if(!flag) {
            delete d_list[i];
        }
    }

    d_list_top = d_list_new_top;
    for(int i = 0; i < d_list_new_top; ++i) {
        d_list[i] = d_list_new[i];
    }
}

void retire_node(LFNode *node){
    d_list[d_list_top] = node;
    d_list_top++;

    if(d_list_top == d_list_max_size){
        free_d_list();
    }
}

struct LFStack{
    std::atomic<LFNode *> top;

    void push(int value){
        LFNode *new_node = new LFNode;
        new_node->data = value;
        bool flag = false;

        do {
            if (flag){
                std::this_thread::yield();
            }
            new_node->next = top.load(std::memory_order_acquire);
        } while((flag = !top.compare_exchange_weak(new_node->next, new_node, std::memory_order_release)));
    }

    int pop(int thread_num){
        LFNode *cur = nullptr;
        LFNode *next_node;
        do {
            if (cur != nullptr){
                std::this_thread::yield();
            }
            do {
                cur = top.load(std::memory_order_relaxed);
                HP[thread_num] = cur;
            } while (cur != top.load(std::memory_order_acquire));

            next_node = cur->next;
            if (cur == nullptr){
                return -1;
            }
        } while (!top.compare_exchange_weak(cur, next_node, std::memory_order_release));

        int data = cur->data;
        HP[thread_num] = nullptr;
        retire_node(cur);
        return data;
    }
};

struct LNode{
    int data = 0;
    LNode *next = nullptr;
};

std::mutex mutex;

struct LStack{
    LNode *top = nullptr;

    void push(int value){
        LNode *new_node = new LNode;
        new_node->data = value;

        mutex.lock();
        new_node->next = top;
        top = new_node;
        mutex.unlock();
    }

    int pop(){
        mutex.lock();
        LNode *cur = top;

        if(cur == nullptr) {
            mutex.unlock();
            return -1;
        }

        int data = cur->data;
        top = cur->next;
        mutex.unlock();

        delete cur;
        return data;
    }
};

//=====================================================================================

LFStack s;
const int N = 100000;
double b[max_threads], unb[max_threads];

void thread_routine_first(int index){
    int i = 0, p = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (i = 0; i < 2 * N; ++i){
        s.push(i);
        p = s.pop(index);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double diff = (end - start).count();
    b[index] = diff / N;
}

void thread_routine_second(int index){
    int i = 0, p = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (i = 0; i < N; ++i){
        s.push(i);
    }
    for (i = 0; i < N; ++i){
        p = s.pop(index);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double diff = (end - start).count();
    unb[index] = diff / N;
}

//=================================================================================

LStack ls;
double lb[max_threads], lunb[max_threads];

void thread_routine_first_lock(int index){
    int i = 0, p = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (i = 0; i < 2 * N; ++i){
        ls.push(i);
        p = ls.pop();
    }
    auto end = std::chrono::high_resolution_clock::now();
    double diff = (end - start).count();
    lb[index] = diff / N;
}

void thread_routine_second_lock(int index){
    int i = 0, p = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (i = 0; i < N; ++i){
        ls.push(i);
    }
    for (i = 0; i < N; ++i){
        p = ls.pop();
    }
    auto end = std::chrono::high_resolution_clock::now();
    double diff = (end - start).count();
    lunb[index] = diff / N;
}

//=================================================================================


int main()
{
    std::printf("%10s | %10s | %10s | %10s | %10s\n", "Threads", "Balanced load", "Unbalanced load", "Mutex balanced load", "Mutex unbalanced load");

    int num_threads[9] = {2, 3, 4, 5, 6, 7, 8, 10, 12};
    std::thread *threads[max_threads];
    for (int i = 0; i < 9; ++i) {
        current_threads = num_threads[i];
        double b_avg = 0.0, unb_avg = 0.0, lb_avg = 0.0, lunb_avg = 0.0;
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j] = new std::thread(thread_routine_first, j);
        }
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j]->join();
            delete(threads[j]);

            b_avg += b[j];
        }
        b_avg /= num_threads[i];

        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j] = new std::thread(thread_routine_second, j);
        }
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j]->join();
            delete(threads[j]);

            unb_avg += unb[j];
        }
        unb_avg /= num_threads[i];

        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j] = new std::thread(thread_routine_first_lock, j);
        }
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j]->join();
            delete(threads[j]);

            lb_avg += lb[j];
        }
        lb_avg /= num_threads[i];

        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j] = new std::thread(thread_routine_second_lock, j);
        }
        for (int j = 0; j < num_threads[i]; ++j) {
            threads[j]->join();
            delete(threads[j]);

            lunb_avg += lunb[j];
        }
        lunb_avg /= num_threads[i];

        std::printf(
                "%10d | %10lf | %10lf | %10lf | %10lf\n",
                num_threads[i],
                b_avg, unb_avg, lb_avg, lunb_avg
        );
    }
}

