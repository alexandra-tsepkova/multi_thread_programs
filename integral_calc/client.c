#include "lib.h"

int main(int argc, char **argv) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        printf("Can't create socket");
        return -1;
    }

    const struct sockaddr_in server_addr = {AF_INET, htons(PORT), inet_addr("127.0.0.1")};
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Connection error\n");
        return -1;
    }
    while(1) {
        struct slice received;

        int mes_size = read(sock_fd, &received, sizeof(received));
        if (mes_size > sizeof(received)) {
            printf("Error while receiving message\n");
            return -1;
        }
        printf("Received %d slice from %lf to %lf\n", received.number, received.a, received.b);

        received.result = integrate(received.a, received.b, 1000);
        if (send(sock_fd, (const void *) &received, sizeof(struct slice), 0) < 0) {
            printf("Error sending\n");
            return 1;
        } else printf("Send counted %d slice \n", received.number);
    }
    close(sock_fd);
    return 0;
}