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
            const char *mensaje = "No se encuentra el archivo a ejecutar";
            write(tuberiaMensajes[1], &mensaje, sizeof(mensaje));
            sem_post(semaforo);
        }

        // Intentar conectar con proceso 3
        sem_t *semaforoCompartido;
        int area = shm_open("/SemaforoCompartido", O_RDWR, 0666);
        if (area == -1) {
            const char *mensaje = "Proceso p3 no parece estar en ejecución";
            write(tuberiaMensajes[1], &mensaje, sizeof(mensaje));
            sem_post(semaforo);

            const char *salida = "exit";
            write(tuberiaMensajes[1], &salida, sizeof(salida));
            sem_post(semaforo);
            return 1;
        }
        semaforoCompartido = mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, area, 0);

        // Enviar ruta a proceso 3
        printf("Enviando ruta a proceso 3\n");
        char *memoriaCompartida;
        int areaMemoriaCompartida = shm_open("/memoriaCompartida", O_RDWR, 0666);
	    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaMemoriaCompartida, 0);

        strcpy(memoriaCompartida, ruta);

        // Descongelar proceso 3
        sem_post(semaforoCompartido);
        sleep(3);

        // Leer la salida del proceso 3
        sem_wait(semaforoCompartido);
        printf("Salida del proceso 3: %s\n", memoriaCompartida);


        /* Desasociar el semáforo y eliminarlo */
        //munmap(semaforoCompartido, sizeof(sem_t));
        //shm_unlink("/SemaforoCompartido");

        // Salir
        const char *salida = "exit";
        write(tuberiaMensajes[1], &salida, sizeof(salida));
        sem_post(semaforo);

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
            char* mensaje;
            read(tuberiaMensajes[0], &mensaje, sizeof(mensaje));
            
            if (mensaje == "exit") {
                break;
            }
            
            printf("Mensaje recibido: %s\n", mensaje);
        }

    }

    close(tuberia[0]);
    close(tuberia[1]);

    close(tuberiaMensajes[0]);
    close(tuberiaMensajes[1]);

    sem_destroy(semaforo);
    munmap(semaforo, sizeof(sem_t));
    return 0;
}