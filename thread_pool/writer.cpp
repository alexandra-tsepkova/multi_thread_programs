#include <thread>
#include <deque>
#include <mutex>
#include <array>
#include <string>
#include <optional>
#include <vector>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>


int main(int argc, char **argv) {
    int fd = open(argv[1], O_WRONLY);

    while (true) {
        char buf[1024];
        int num = read(0, buf, 1024);
        write(fd, buf, num);
    }
}