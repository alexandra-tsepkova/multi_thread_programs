#include <thread>
#include <deque>
#include <mutex>
#include <array>
#include <string>
#include <optional>
#include <vector>
#include <iostream>
#include <atomic>

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define this_queue worker_queues_ptr->at(index)

const int NUMBER_OF_WORKERS = 2;
const int MAX_EVENTS = 16;

std::atomic_bool thread_exit = false;

struct WorkerQueue {
    std::deque<std::string> deque;
    std::mutex mutex;
};


std::optional<std::string> try_get_task(WorkerQueue& queue) {
    std::lock_guard lock(queue.mutex);

    if (!queue.deque.empty()) {
        std::string task = queue.deque.front();
        queue.deque.pop_front();
        return task;
    }

    return {};
}


void send_task(WorkerQueue& queue, std::string task) {
    std::lock_guard lock(queue.mutex);
    queue.deque.push_back(task);
}


int get_queue_size(WorkerQueue& queue) {
    std::lock_guard lock(queue.mutex);
    return queue.deque.size();
}


void worker_routine(int index, std::array<WorkerQueue, NUMBER_OF_WORKERS>* worker_queues_ptr) {

    while (true) {
        std::string task;

        while (true) {
            auto try_result = try_get_task(this_queue);

            if (try_result.has_value()) {
                task = try_result.value();
                break;
            }

            // Work stealing
            int steal_index = -1;
            int max_size = 0;
            for (int i = 0; i < NUMBER_OF_WORKERS; ++i){
                if (i == index) continue;
                int cur_size = get_queue_size(worker_queues_ptr->at(i));
                if (cur_size > max_size) steal_index = i;
                max_size = cur_size;
            }
            if (steal_index > -1) {
                try_result = try_get_task(worker_queues_ptr->at(steal_index));

                if (try_result.has_value()) {
                    task = try_result.value();
                    break;
                }
            }
            if(thread_exit.load()) return;
            // Backoff
            std::this_thread::yield();
        }

        // do something here
        if (index == NUMBER_OF_WORKERS - 1){
            sleep(3);
        }
        std::cout << "I am thread number " << index << ". I did this: " << task << std::endl;
    }
}


int main(int argc, char **argv) {
    std::array<WorkerQueue, NUMBER_OF_WORKERS> worker_queues;
    std::array<std::thread, NUMBER_OF_WORKERS> worker_threads;
    int next_worker = 0;

    // Initialise workers
    int current_index = 0;
    for (auto &thread : worker_threads) {
        thread = std::thread(worker_routine, current_index++, &worker_queues);
    }

    // Initialise input descriptors
    std::vector<int> fds(argc);
    for (int i = 1; i < argc; ++i) {

        fds[i - 1] = open(argv[i], O_RDONLY);
        if (fds[i - 1] < 0){
            std::cout << "cannot open " << argv[i] << std::endl;
            return -1;
        }
    }

    fds[argc - 1] = STDIN_FILENO;

    // Initialise epoll

    int epoll_fd = epoll_create(argc);
    for (int i = 0; i < argc; ++i) {
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = fds[i];
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[i], &event);
    }

    // Loop

    bool exit_flag = false;
    while (true) {
        struct epoll_event events[MAX_EVENTS];
        int epoll_result = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        if (epoll_result == -1){
            std::cout << "epoll wait error" << std::endl;
            return -1;
        }

        for (int i = 0; i < epoll_result; ++i) {
            int fd = events[i].data.fd;
            if (fd == STDIN_FILENO){
                exit_flag = true;
                thread_exit.store(true);
                break;

            }
            char buf[1024];
            int bytes_read = read(fd, buf, 1023);
            buf[bytes_read] = '\0';

            send_task(worker_queues[next_worker], std::string(buf));
            next_worker = (next_worker + 1) % NUMBER_OF_WORKERS;
        }
        if (exit_flag) break;
    }

    // Wrapping up
    close(epoll_fd);
    for (int i = 0; i < argc - 1; ++i) {
        close(fds[i]);
    }
    for (auto &thread:worker_threads){
        thread.join();
    }
    return 0;
}