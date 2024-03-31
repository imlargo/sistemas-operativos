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

// gcc -pthread -o program1 program1.c -lpthread -lrt

int main(int argc, char const *argv[]) {

    int tuberiaEnvio[2], tuberiaRecepcion[2];
    if (pipe(tuberiaEnvio) == -1) {
        perror("Error en programa 1 al crear tuberia");
        return -1;
    }
    if (pipe(tuberiaRecepcion) == -1) {
        perror("Error en programa 1 al crear tuberia");
        return -1;
    }

    // Crear semáforos
    sem_unlink("/semaforoP1");
    sem_t *semaforoP1;
    semaforoP1 = sem_open("/semaforoP1", O_CREAT, 0666, 0);
    if (semaforoP1 == SEM_FAILED) {
        perror("Error en programa 1 al intentar crear semáforo");
        return -1;
    }

    sem_unlink("/semaforoP2");
    sem_t *semaforoP2;
    semaforoP2 = sem_open("/semaforoP2", O_CREAT, 0666, 0);
    if (semaforoP2 == SEM_FAILED) {
        perror("Error en programa 1 al intentar crear semáforo");
        return -1;
    }

    pid_t pid = fork();

    if (pid > 0) {
        // Proceso padre
        close(tuberiaEnvio[0]);
        close(tuberiaRecepcion[1]);

        const char *ruta;

        if (argc == 1) {
            printf("Uso: p1 /ruta/al/ejecutable\n");

            ruta = "exit";
            write(tuberiaEnvio[1], &ruta, sizeof(ruta));
            sem_post(semaforoP2);

            // Liberar memoria
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP1");

            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);

            return 0;
        }

        ruta = argv[1];

        write(tuberiaEnvio[1], &ruta, sizeof(ruta));
        sem_post(semaforoP2);

        while (1) {
            sem_wait(semaforoP1);

            char mensaje[4096];
            int bytesRead = read(tuberiaRecepcion[0], mensaje, sizeof(mensaje) - 1);
            if (bytesRead >= 0) {
                mensaje[bytesRead] = '\0';
            }
            if (strcmp(mensaje, "exit") == 0) {
                sem_post(semaforoP2);
                break;
            }
            printf("Mensaje recibido: %s", mensaje);
            
            sem_post(semaforoP2);
        }

        sem_close(semaforoP2);
        sem_close(semaforoP1);
        sem_unlink("/semaforoP1");

        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);

    } else if (pid == 0) {
        // Proceso hijo
        
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);

        sem_wait(semaforoP2);

        char* ruta;
        read(tuberiaEnvio[0], &ruta, sizeof(ruta));

        // Intentar conectar con proceso 3 primero
        char *memoriaCompartida;
        int areaCompartida = shm_open("/memoriaCompartida", O_RDWR, 0666);

        if (strcmp(ruta, "exit") == 0) {
            // liberar toda la memoria
            if (areaCompartida != -1) {
                memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);
                
                sem_t *semaforoP3;
                semaforoP3 = sem_open("/semaforoP3", O_RDWR);
                sprintf(memoriaCompartida, "%s", "exit");

                sem_post(semaforoP3);
                sem_close(semaforoP3);

                munmap(memoriaCompartida, 4096);
            }

            // Liberar memoria
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP2");

            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);

            return 0;
        }

        if (access(ruta, X_OK) != 0) {
            const char *mensaje = "No se encuentra el archivo a ejecutar\n";
            write(tuberiaRecepcion[1], mensaje, strlen(mensaje));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);

            const char *salidaM = "exit";
            write(tuberiaRecepcion[1], salidaM, strlen(salidaM));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);

            if (areaCompartida != -1) {
                memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);
                
                sem_t *semaforoP3;
                semaforoP3 = sem_open("/semaforoP3", O_RDWR);
                sprintf(memoriaCompartida, "%s", "exit");

                sem_post(semaforoP3);
                sem_close(semaforoP3);

                munmap(memoriaCompartida, 4096);
            }

            // Liberar memoria
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP2");

            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);
            return 0;
        }
        
        if (areaCompartida == -1) {
            const char *mensaje = "Proceso p3 no parcece estar en ejecución\n";
            write(tuberiaRecepcion[1], mensaje, strlen(mensaje));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);
            
            const char *salidaM = "exit";
            write(tuberiaRecepcion[1], salidaM, strlen(salidaM));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);

            // Liberar memoria
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP2");

            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);
            return 0;
        }
	    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);

        sem_t *semaforoP3;
        semaforoP3 = sem_open("/semaforoP3", O_RDWR);
        
        // Escribir la ruta en memoria compartida
        sprintf(memoriaCompartida, "%s", ruta);

        // Descongelar proceso 3
        sem_post(semaforoP3);

        // Esperar y leer la salida del proceso 3
        sem_wait(semaforoP2);

        char buffer[4096];
        strcpy(buffer, memoriaCompartida);
        write(tuberiaRecepcion[1], buffer, strlen(buffer));

        // Esperar que el proceso padre termine de imprimir el resultado
        sem_post(semaforoP1);
        sem_wait(semaforoP2);

        // Salir
        const char *salida = "exit";
        write(tuberiaRecepcion[1], salida, strlen(salida));
        sem_post(semaforoP1);
        sem_wait(semaforoP2);

        // Enviar señal a proceso 3 para eliminar memoria
        sem_post(semaforoP3);
        sem_close(semaforoP3);

        sem_close(semaforoP2);
        sem_close(semaforoP1);
        sem_unlink("/semaforoP2");

        munmap(memoriaCompartida, 4096);

        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);
    }

    return 0;
}