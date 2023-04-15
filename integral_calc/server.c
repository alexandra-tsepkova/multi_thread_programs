#include "lib.h"

#define MAX_WORKERS 10


double reduce(struct slice *slices, int num_slices){
    double res = 0.0;
    for (int i = 0; i < num_slices; ++i){
        res += slices[i].result;
    }
    return res;
}
int N;
int last_checked = 0;
struct slice* array_slices;
int* status;
int* check;

int get_next_slice() {
    pthread_mutex_lock(&mutex);
    while (status[last_checked]) {
        last_checked++;
        if (last_checked == N) {
            last_checked = 0;
        }
    }
    pthread_mutex_unlock(&mutex);
    return last_checked;
}

void mark_slice_as_ready(int i, int *flag_cnt) {
    pthread_mutex_lock(&mutex);
    if (status[i] != 1) {
        status[i] = 1;
        *flag_cnt = 1;
    } else *flag_cnt = -1;
    pthread_mutex_unlock(&mutex);
}

void* thread_routine(void *args){
    int newsock_fd = *((int*) args);
    int flag_cnt = 0;
    while(1) {
        int flags = 1;

        if (setsockopt(newsock_fd, SOL_SOCKET, SO_KEEPALIVE, (void *) &flags, sizeof(flags))) {
            printf("Can't setsockopt\n");
            exit(0);
        }

        if (write(newsock_fd, (const void *) &array_slices[get_next_slice()], sizeof(struct slice)) < 0) {
            printf("Error sending\n");
        } else {
            struct slice counted;
            if (read(newsock_fd, &counted, sizeof(struct slice)) < 0) {
                printf("Error reading\n");
            }
            printf("Server received counted %d - result %lf\n", counted.number, counted.result);
            mark_slice_as_ready(counted.number, &flag_cnt);
            if (flag_cnt == 1) {
                array_slices[counted.number].result = counted.result;
            }
        }
        if (memcmp(status, check, N * sizeof(int)) == 0) return 0;
    }
}

int main(int argc, char **argv) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        printf("Can't create socket\n");
        return -1;
    }
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    const struct sockaddr_in sock_addr = {AF_INET, htons(PORT), inet_addr("127.0.0.1")};

    if (bind(sock_fd, (struct sockaddr *) (&sock_addr), sizeof(sock_addr)) < 0) {
        printf("Can't assign address to a socket\n");
        close(sock_fd);
        return -1;
    }

    if (argc < 3){
        printf("Usage: ./program_name a b\n");
        return -1;
    }
    double a, b, cur;
    a = atof(argv[1]);
    b = atof(argv[2]);
    cur = a;

    N = (int)ceilf(fabs(b - a)/STEP);
    array_slices = malloc(N * sizeof(struct slice));
    status = malloc(N * sizeof(int));
    check = malloc(N * sizeof(int));

    for (int i = 0; i < N; ++i){
        array_slices[i].number = i;
        array_slices[i].a = cur;
        array_slices[i].b = (cur + STEP) < b ? cur + STEP : b;
        cur += STEP;
        status[i] = 0;
        check[i] = 1;
    }
    int i = 0;
    if (listen(sock_fd, 5) < 0) {
        printf("Can't connect to socket\n");
        close(sock_fd);
        return -1;
    }

    int last_used_thread;
    pthread_t thread_ids[MAX_WORKERS];
    while(1){

        struct sockaddr_in con_sock_addr;
        socklen_t con_socklen;
        sleep(1);
        int newsock_fd = accept(sock_fd, (struct sockaddr*)(&con_sock_addr), &con_socklen);
        if(newsock_fd <= 0) {
            if (memcmp(status, check, N * sizeof(int)) == 0) break;
        } else {
            pthread_create(thread_ids + last_used_thread, NULL, thread_routine, (void*) &newsock_fd);
            last_used_thread++;
        }
    }

    double result = reduce(array_slices, N);
    printf("Sum of all results equals %lf\n", result);
    return 0;
}