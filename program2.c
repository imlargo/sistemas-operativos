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

    shm_unlink("/SemaforoCompartido");

    // Crear area de memoria para el semaforo compartido
    sem_t *semaforoCompartido;
    int area = shm_open("/SemaforoCompartido", O_CREAT | O_RDWR, 0666);
    if (area == -1) {
        perror("shm_open");
        return 1;
    }
    ftruncate(area, sizeof(sem_t));
    semaforoCompartido = mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, area, 0);
    if (semaforoCompartido == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Crear area de memoria para buffer compartido
    char *memoriaCompartida;
    int areaMemoriaCompartida = shm_open("/memoriaCompartida", O_CREAT | O_RDWR, 0666);
    if (areaMemoriaCompartida == -1) {
        perror("shm_open");
        return 1;
    }
    ftruncate(areaMemoriaCompartida, 4096);
    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaMemoriaCompartida, 0);
    if (memoriaCompartida == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Iniciar semaforo y esperar
    sem_init(semaforoCompartido, 1, 0);
    sem_wait(semaforoCompartido);

    // Leer la ruta del buffer compartido
    char *ruta = malloc(256 * sizeof(char));
    strcpy(ruta, memoriaCompartida);
    printf("Ruta: %s\n", ruta);

    int tuberiaSalida[2];
    pipe(tuberiaSalida);

    pid_t pid = fork();

    if (pid == 0)
    {
        // Proceso hijo
        close(tuberiaSalida[1]);

        int buffer[255];
        read(tuberiaSalida[0], buffer, sizeof(buffer));
        char *mensaje = (char *)buffer;

        strcpy(memoriaCompartida, mensaje);
        sem_post(semaforoCompartido);

        sem_wait(semaforoCompartido);
        /* Desasociar el semáforo y eliminarlo */
        printf("Eliminadno memoria\n");
        //munmap(semaforoCompartido, sizeof(sem_t)); // Unmap semaforoCompartido
        //munmap(memoriaCompartida, 4096); // Unmap memoriaCompartida
        free(ruta); // Free allocated memory for ruta
    } else {
        // Proceso padre
        close(tuberiaSalida[0]);

        dup2(tuberiaSalida[1], STDOUT_FILENO);
        execl(ruta, ruta, (char *) NULL);

        printf("Error al ejecutar el comando\n");
    }

    return 0;
}