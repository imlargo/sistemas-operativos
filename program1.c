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

// gcc -pthread -o p1 p1.c -lpthread -lrt

int main(int argc, char const *argv[]) {

    // Crear tuberias para envío y recepción de mensajes entre proceso 1 y 2
    int tuberiaEnvio[2], tuberiaRecepcion[2];
    if (pipe(tuberiaEnvio) == -1) {
        perror("Error en programa 1 al crear tuberia");
        return -1;
    }
    if (pipe(tuberiaRecepcion) == -1) {
        perror("Error en programa 1 al crear tuberia");
        return -1;
    }

    sem_unlink("/semaforoP1");
    // Crear semáforo para proceso 1
    sem_t *semaforoP1;
    semaforoP1 = sem_open("/semaforoP1", O_CREAT, 0666, 0);
    if (semaforoP1 == SEM_FAILED) {
        perror("Error en programa 1 al intentar crear semáforo");

        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);
        return -1;
    }
    
    sem_unlink("/semaforoP2");
    // Crear semáforo para proceso 2
    sem_t *semaforoP2;
    semaforoP2 = sem_open("/semaforoP2", O_CREAT, 0666, 0);
    if (semaforoP2 == SEM_FAILED) {
        perror("Error en programa 1 al intentar crear semáforo");

        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);

        sem_close(semaforoP1);
        sem_unlink("/semaforoP1");
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Error al crear proceso hijo con fork()");
        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);

        sem_close(semaforoP2);
        sem_close(semaforoP1);
        sem_unlink("/semaforoP1");
        sem_unlink("/semaforoP2");
        return -1;
    }

    if (pid > 0) {
        // Proceso padre (PROCESO #1)
        close(tuberiaEnvio[0]);
        close(tuberiaRecepcion[1]);

        const char *ruta;

        // Verificar si solo se le pasó un argumento
        if (argc == 1) {
            printf("Uso: p1 /ruta/al/ejecutable\n");

            // Enviar señal de salida a proceso 2
            ruta = "exit";
            write(tuberiaEnvio[1], &ruta, sizeof(ruta));
            sem_post(semaforoP2);

            // Liberar recursos
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP1");
            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);

            return 0;
        }
        
        // Guardar el valor del argumento en la variable ruta
        ruta = argv[1];

        // Enviar la ruta al proceso 2
        write(tuberiaEnvio[1], &ruta, sizeof(ruta));
        sem_post(semaforoP2);

        // Entrar en bucle infinito para recibir mensajes
        while (1) {
            sem_wait(semaforoP1);

            char mensaje[4096];
            int bytesRead = read(tuberiaRecepcion[0], mensaje, sizeof(mensaje) - 1);
            if (bytesRead >= 0) {
                mensaje[bytesRead] = '\0';
            }

            // En caso de recibir señal de salida, sale del bucle
            if (strcmp(mensaje, "exit") == 0) {
                sem_post(semaforoP2);
                break;
            }
            printf("Mensaje recibido: %s", mensaje);
            
            sem_post(semaforoP2);
        }

        // Liberar recursos
        sem_close(semaforoP2);
        sem_close(semaforoP1);
        sem_unlink("/semaforoP1");

        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);

    } else if (pid == 0) {
        // Proceso hijo (PROCESO #2)
        
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);

        // Esperar a recibir la ruta del proceso 1
        sem_wait(semaforoP2);

        char* ruta;
        read(tuberiaEnvio[0], &ruta, sizeof(ruta));

        // Intentar conectar con la memoria compartida del proceso 3
        char *memoriaCompartida;
        int areaCompartida = shm_open("/memoriaCompartida", O_RDWR, 0666);

        if (strcmp(ruta, "exit") == 0) {
            /*
                Recibir señal de salida si el proceso 1 detecta que no se le pasó un argumento,
                y enviar señal de salida al proceso 3 mediante memoria compartida en caso de que exista
            */ 
            
            if (areaCompartida != -1) {

                // SI el proceso 3 está en ejecución, enviar señal de salida

                memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);
                
                sem_t *semaforoP3;
                semaforoP3 = sem_open("/semaforoP3", O_RDWR);
                sprintf(memoriaCompartida, "%s", "exit");

                sem_post(semaforoP3);
                sem_close(semaforoP3);

                munmap(memoriaCompartida, 4096);
            }

            // Liberar recursos

            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP2");

            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);

            return 0;
        }

        // Verificar si el archivo a ejecutar existe
        if (access(ruta, X_OK) != 0) {
            
            // Enviar mensaje mediante tuberia con proceso 1
            const char *mensaje = "No se encuentra el archivo a ejecutar\n";
            write(tuberiaRecepcion[1], mensaje, strlen(mensaje));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);

            // Enviar señal de salida al proceso 1
            const char *salidaM = "exit";
            write(tuberiaRecepcion[1], salidaM, strlen(salidaM));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);

            //  Enviar señal de salida al proceso 3 en caso de que este en ejecución para que libere los recursos
            if (areaCompartida != -1) {
                memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);
                
                sem_t *semaforoP3;
                semaforoP3 = sem_open("/semaforoP3", O_RDWR);
                sprintf(memoriaCompartida, "%s", "exit");

                sem_post(semaforoP3);
                sem_close(semaforoP3);

                munmap(memoriaCompartida, 4096);
            }

            // Liberar recursos
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP2");

            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);
            return 0;
        }
        
        // Verificar si el proceso 3 está en ejecución verificando si la memoria compartida existe
        if (areaCompartida == -1) {

            // Enviar mensaje mediante tuberia con proceso 1
            const char *mensaje = "Proceso p3 no parece estar en ejecución\n";
            write(tuberiaRecepcion[1], mensaje, strlen(mensaje));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);
            
            // Enviar señal de salida al proceso 1
            const char *salidaM = "exit";
            write(tuberiaRecepcion[1], salidaM, strlen(salidaM));
            sem_post(semaforoP1);
            sem_wait(semaforoP2);

            // Liberar recursos
            sem_close(semaforoP2);
            sem_close(semaforoP1);
            sem_unlink("/semaforoP2");

            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);
            return 0;
        }

        // Mapear memoria compartida y conectar con semáforo del proceso 3
	    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaCompartida, 0);

        if (memoriaCompartida == MAP_FAILED) {
            perror("Error al mapear memoria compartida");

            close(tuberiaEnvio[0]);
            close(tuberiaEnvio[1]);
            close(tuberiaRecepcion[0]);
            close(tuberiaRecepcion[1]);

            sem_close(semaforoP1);
            sem_close(semaforoP2);
            sem_unlink("/semaforoP1");
            sem_unlink("/semaforoP2");

            shm_unlink("/memoriaCompartida");

            return -1;
        }

        sem_t *semaforoP3;
        semaforoP3 = sem_open("/semaforoP3", O_RDWR);
        
        // Escribir la ruta en memoria compartida y enviar señal al proceso 3
        sprintf(memoriaCompartida, "%s", ruta);
        sem_post(semaforoP3);

        // Esperar el resultado de ejecución del proceso 3 y leer la salida
        sem_wait(semaforoP2);
        char buffer[4096];
        strcpy(buffer, memoriaCompartida);

        // Enviar el resultado de ejecución mediante tuberia con proceso 1
        write(tuberiaRecepcion[1], buffer, strlen(buffer));
        sem_post(semaforoP1);
        sem_wait(semaforoP2);

        // Enviar señal de salida al proceso 1
        const char *salida = "exit";
        write(tuberiaRecepcion[1], salida, strlen(salida));
        sem_post(semaforoP1);
        sem_wait(semaforoP2);

        // Enviar señal a proceso 3 para terminar su ejecución y que libere recursos
        sem_post(semaforoP3);
        
        // Liberar recursos
        sem_close(semaforoP1);
        sem_close(semaforoP2);
        sem_close(semaforoP3);
        sem_unlink("/semaforoP2");

        munmap(memoriaCompartida, 4096);

        close(tuberiaEnvio[0]);
        close(tuberiaEnvio[1]);
        close(tuberiaRecepcion[0]);
        close(tuberiaRecepcion[1]);
    }

    return 0;
}