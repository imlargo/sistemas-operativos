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
    semaforo = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(semaforo, 1, 0);

    pid_t pid = fork();

    if (pid == 0) {
        // Proceso hijo
        close(tuberia[1]);
        close(tuberiaMensajes[0]);

        char* ruta;
        read(tuberia[0], &ruta, sizeof(ruta));

        if (access(ruta, X_OK) != 0) {
            const char *mensaje = "No se encuentra el archivo a ejecutar\n";
            write(tuberiaMensajes[1], mensaje, strlen(mensaje));
            sem_post(semaforo);
        }

        // Intentar conectar con proceso 3
        sem_t *semaforoCompartido;
        int area = shm_open("/SemaforoCompartido", O_RDWR, 0666);
        if (area == -1) {
            const char *mensaje = "Proceso p3 no parece estar en ejecución\n";
            write(tuberiaMensajes[1], mensaje, strlen(mensaje));
            sem_post(semaforo);

            sleep(1);
            
            const char *salidaM = "exit";
            write(tuberiaMensajes[1], salidaM, strlen(salidaM));
            sem_post(semaforo);
            return 0;
        }
        semaforoCompartido = mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, area, 0);

        // Enviar ruta a proceso 3
        char *memoriaCompartida;
        int areaMemoriaCompartida = shm_open("/memoriaCompartida", O_RDWR, 0666);
	    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaMemoriaCompartida, 0);

        // Escribir en memoria compartida
        sprintf(memoriaCompartida, "%s", ruta);

        // Descongelar proceso 3
        sem_post(semaforoCompartido);

        sleep(1);

        // Leer la salida del proceso 3
        sem_wait(semaforoCompartido);

        // Leer de memoria compartida
        char buffer[4096];
        strcpy(buffer, memoriaCompartida);
        write(tuberiaMensajes[1], buffer, strlen(buffer));
        sleep(1);
        sem_post(semaforo);
        sleep(1);

        // Salir
        const char *salida = "exit";
        write(tuberiaMensajes[1], salida, strlen(salida));
        sem_post(semaforo);

        // Liberar memoria
        munmap(memoriaCompartida, 4096);
        shm_unlink("/memoriaCompartida");
        munmap(semaforoCompartido, sizeof(sem_t));
        shm_unlink("/SemaforoCompartido");

    } else {
        // Proceso padre
        close(tuberia[0]);
        close(tuberiaMensajes[1]);

        const char *ruta;

        if (argc == 1) {
            char *base = "/bin/ls";
            printf("Uso: %s\n", base);
            ruta = base;
        } else {
            ruta = argv[1];
        }
        write(tuberia[1], &ruta, sizeof(ruta));

        while (1) {
            sem_wait(semaforo);

            char mensaje[4096];
            int bytesRead = read(tuberiaMensajes[0], mensaje, sizeof(mensaje) - 1);
            if (bytesRead >= 0) {
                mensaje[bytesRead] = '\0'; // Aseguramos que el buffer sea una cadena válida en C
            }
            if (strcmp(mensaje, "exit") == 0) {
                break;
            }
            printf("Mensaje recibido: %s", mensaje);
        }

        sem_destroy(semaforo);
        munmap(semaforo, sizeof(sem_t));
    }

    close(tuberia[0]);
    close(tuberia[1]);
    close(tuberiaMensajes[0]);
    close(tuberiaMensajes[1]);

    return 0;
}