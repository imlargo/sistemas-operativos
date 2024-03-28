#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

int main() {
    
    /* Conectar */
    sem_t *semaforoCompartido;
    int area = shm_open("/SemaforoCompartido", O_RDWR, 0666);
    if (area == -1) {
        printf("Error en shm_open\n");
        return 1;
    }
    printf("Area: %d\n", area);
    
    semaforoCompartido = mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, area, 0);
    if (semaforoCompartido == MAP_FAILED) {
        printf("Error en mmap\n");
        return 1;
    }

    printf("Semaforo conectado\n");
    int semv;
    sem_getvalue(semaforoCompartido, &semv);
    printf("Valor del semáforo: %d\n", semv);

    printf("Despertando primer proceso\n");
    sem_post(semaforoCompartido);

    /* Desasociar el semáforo y eliminarlo */
    munmap(semaforoCompartido, sizeof(sem_t));
    shm_unlink("/SemaforoCompartido");

    return 0;
}