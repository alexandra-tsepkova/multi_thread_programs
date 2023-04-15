//#define _GNU_SOURCE
#include <math.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#define PORT 65535
#define STEP 1.
#define MAX_NODES

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct slice{
    int number;
    double a;
    double b;
    double result;
};

double f(double x){
    return x;
}

double integrate(double a, double b, int steps){
    double res = 0;
    double step = (b - a)/steps;
    while ((b - a) > 1e-5){
        res += f(a + step/2) * step;
        a += step;
    }
    return res;
}

void print_arr(int* arr, int N){
    for (int i = 0; i < N; ++i){
        printf("%d ", arr[i]);
    }
    printf("\n");
}



