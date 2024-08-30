#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/shm.h>

int main()
{

    int a = 100;
    int b = 200;
    int c = 300;
    int d = 400;
    int e = 500;

    int *p1 = malloc(sizeof(int));
    int *p2 = malloc(sizeof(int));
    int *p3 = malloc(sizeof(int));
    int *p4 = malloc(sizeof(int));
    int *p5 = malloc(sizeof(int));

    memcpy(p1, &a, sizeof(int));
    memcpy(p2, &b, sizeof(int));
    memcpy(p3, &c, sizeof(int));
    memcpy(p4, &d, sizeof(int));
    memcpy(p5, &e, sizeof(int));

    printf("%p heap[0] = %d\n", (void *)&p1, *p1);
    printf("%p heap[1] = %d\n", (void *)&p2, *p2);
    printf("%p heap[2] = %d\n", (void *)&p3, *p3);
    printf("%p heap[3] = %d\n", (void *)&p4, *p4);
    printf("%p heap[4] = %d\n", (void *)&p5, *p5);

    free(p1);
    free(p2);
    free(p3);
    free(p4);
    free(p5);

    return 0;
}