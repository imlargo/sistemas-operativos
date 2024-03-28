#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/shm.h>


int main(int argc, char const *argv[]) {

    int tuberia[2], tuberiaMensajes[2];
    pipe(tuberia);
    pipe(tuberiaMensajes);

    // Crear semáforo
    sem_t *semaforo;
    semaforo = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
    sem_init(semaforo, 1, 0);

    munmap(semaforo, sizeof(sem_t)); // Liberar el área de memoria compartida
    return 0;
}