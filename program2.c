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
    ftruncate(area, sizeof(sem_t));
    semaforoCompartido = mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, area, 0);

    // Crear area de memoria para buffer compartido
    char *memoriaCompartida;
    int areaMemoriaCompartida = shm_open("/memoriaCompartida", O_CREAT | O_RDWR, 0666);
    ftruncate(areaMemoriaCompartida, 4096);
    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaMemoriaCompartida, 0);

    // Iniciar semaforo y esperar
    sem_init(semaforoCompartido, 1, 0);
    sem_wait(semaforoCompartido);

    // Leer la ruta del buffer compartido
    char ruta[4096];
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

        sprintf(memoriaCompartida, "%s", mensaje);
        sem_post(semaforoCompartido);

        // liberar toda la memoria
        munmap(semaforoCompartido, sizeof(sem_t));
        shm_unlink("/SemaforoCompartido");

    } else {
        // Proceso padre
        close(tuberiaSalida[0]);

        dup2(tuberiaSalida[1], STDOUT_FILENO);
        execl(ruta, ruta, (char *) NULL);

        printf("Error al ejecutar el comando\n");
    }

    return 0;
}