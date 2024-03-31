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

    shm_unlink("/memoriaCompartida");
    char *memoriaCompartida;
    int areaMemoriaCompartida = shm_open("/memoriaCompartida", O_CREAT | O_RDWR, 0666);
    ftruncate(areaMemoriaCompartida, 4096);
    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaMemoriaCompartida, 0);

    // Iniciar semaforo y esperar
    sem_unlink("/semaforoPr2");
    sem_t *semaforoPr2;
    semaforoPr2 = sem_open("/semaforoPr2", O_CREAT, 0666, 0);

    sem_wait(semaforoPr2);

    sem_t *semH;
    semH = sem_open("/semaforoHijo", O_RDWR);

    // Leer la ruta del buffer compartido
    char ruta[4096];
    strcpy(ruta, memoriaCompartida);
    printf("Ruta: %s\n", ruta);

    int tuberiaSalida[2];
    pipe(tuberiaSalida);

    pid_t pid = fork();

    if (pid == 0) {
        // Proceso hijo
        close(tuberiaSalida[1]);

        int buffer[255];
        read(tuberiaSalida[0], buffer, sizeof(buffer));
        char *mensaje = (char *)buffer;

        // Enviar datos a memoria y despertar proceso 1
        sprintf(memoriaCompartida, "%s", mensaje);
        sem_post(semH);
        sem_wait(semaforoPr2);

        // liberar toda la memoria
        sem_close(semH);
        sem_close(semaforoPr2);
        sem_unlink("/semaforoPr2");
        
        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");

    } else {
        // Proceso padre
        close(tuberiaSalida[0]);

        dup2(tuberiaSalida[1], STDOUT_FILENO);
        execl(ruta, ruta, (char *) NULL);

        printf("Error al ejecutar el comando\n");
        return 0;
    }

    return 0;
}