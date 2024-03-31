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
    int areaCompartida = shm_open("/memoriaCompartida", O_CREAT | O_RDWR, 0666);
    ftruncate(areaCompartida, 4096);
    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);

    // Iniciar semaforo y esperar
    sem_unlink("/semaforoP3");
    sem_t *semaforoP3;
    semaforoP3 = sem_open("/semaforoP3", O_CREAT, 0666, 0);

    sem_wait(semaforoP3);

    // Leer la ruta del buffer compartido
    char ruta[4096];
    strcpy(ruta, memoriaCompartida);

    if (strcmp(ruta, "exit") == 0) {
        // liberar toda la memoria
        sem_close(semaforoP3);
        sem_unlink("/semaforoP3");
        
        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");

        return 0;
    }

    printf("Ruta: %s\n", ruta);

    sem_t *semaforoP2;
    semaforoP2 = sem_open("/semaforoP2", O_RDWR);

    int tuberia[2];
    pipe(tuberia);

    pid_t pid = fork();

    if (pid == 0) {
        // Proceso hijo
        close(tuberia[1]);

        int buffer[255];
        read(tuberia[0], buffer, sizeof(buffer));
        char *mensaje = (char *)buffer;

        // Enviar datos a memoria y despertar proceso 1
        sprintf(memoriaCompartida, "%s", mensaje);
        sem_post(semaforoP2);
        sem_wait(semaforoP3);

        // liberar toda la memoria
        sem_close(semaforoP2);
        sem_close(semaforoP3);
        sem_unlink("/semaforoP3");
        
        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");

        close(tuberia[0]);
        close(tuberia[1]);

    } else {
        // Proceso padre
        close(tuberia[0]);
        sem_close(semaforoP2);
        sem_close(semaforoP3);

        dup2(tuberia[1], STDOUT_FILENO);
        execl(ruta, ruta, (char *) NULL);

        // SI el programa falla:
        printf("Error al ejecutar el comando\n");
        
        close(tuberia[0]);
        close(tuberia[1]);
        return 0;
    }

    return 0;
}