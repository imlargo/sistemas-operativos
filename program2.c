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

// gcc -pthread -o p2 p2.c -lpthread -lrt

int main(int argc, char const *argv[]) {

    // Crear memoria compartida
    shm_unlink("/memoriaCompartida");
    char *memoriaCompartida;
    int areaCompartida = shm_open("/memoriaCompartida", O_CREAT | O_RDWR, 0666);
    if (areaCompartida == -1) {
        perror("Error al crear memoria compartida");
        return -1;
    }
    if (ftruncate(areaCompartida, 4096) == -1) {
        perror("Error al truncar memoria compartida");
        return -1;
    }
    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);
    if (memoriaCompartida == MAP_FAILED) {
        perror("Error al mapear memoria compartida");
        return -1;
    }

    // Iniciar semáforo para sincronización
    sem_unlink("/semaforoP3");
    sem_t *semaforoP3;
    semaforoP3 = sem_open("/semaforoP3", O_CREAT, 0666, 0);
    if (semaforoP3 == SEM_FAILED) {
        perror("Error al abrir semáforo");
        return -1;
    }

    // Esperar señal de proceso 2 para continuar
    sem_wait(semaforoP3);

    // Leer la ruta del programa a ejecutar desde la memoria compartida
    char ruta[4096];
    strcpy(ruta, memoriaCompartida);

    // En caso de recibir señal de salida, liberar recursos
    if (strcmp(ruta, "exit") == 0) {

        // Liberar recursos
        sem_close(semaforoP3);
        sem_unlink("/semaforoP3");

        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");

        return 0;
    }

    printf("Ruta: %s\n", ruta);

    // Conectar con el semáforo del proceso 2
    sem_t *semaforoP2;
    semaforoP2 = sem_open("/semaforoP2", O_RDWR);
    if (semaforoP2 == SEM_FAILED) {
        perror("Error al abrir semáforo");
        return -1;
    }

    // Crear tubería para redirigir salida de programa a ejecutar
    int tuberia[2];
    if (pipe(tuberia) == -1) {
        perror("Error al crear tubería");
        return -1;
    }

    pid_t pid = fork();

    if (pid == 0) {
        // Proceso hijo
        close(tuberia[1]);

        // Leer datos de la tubería
        int buffer[255];
        read(tuberia[0], buffer, sizeof(buffer));
        char *mensaje = (char *)buffer;

        // Guardar resultado de ejecución en memoria compartida y enviar señal al proceso 2
        sprintf(memoriaCompartida, "%s", mensaje);
        sem_post(semaforoP2);

        // Esperar que el proceso 2 permita continuar
        sem_wait(semaforoP3);

        // Liberar recursos
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

        // Redirigir salida del programa a ejecutar mediante tubería
        if (dup2(tuberia[1], STDOUT_FILENO) == -1) {
            perror("Error al redirigir salida");
            return -1;
        }

        // Ejecutar programa con la ruta recibida
        if (execl(ruta, ruta, (char *) NULL) == -1) {
            perror("Error al ejecutar el comando");

            close(tuberia[0]);
            close(tuberia[1]);
            return -1;
        }
    }

    return 0;
}