#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>


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

        if (1) {
            const char *mensaje = "Proceso 3 no iniciado";
            write(tuberiaMensajes[1], &mensaje, sizeof(mensaje));
            sem_post(semaforo);
        }

    } else {
        // Proceso padre
        close(tuberia[0]);
        close(tuberiaMensajes[1]);

        const char *ruta;

        if (argc == 1) {
            printf("Uso: p1 /bin/ls.\n");
            ruta = "/bin/ls";
        } else {
            ruta = argv[1];
        }

        write(tuberia[1], &ruta, sizeof(ruta));

        while (1) {
            sem_wait(semaforo);
            char* mensaje;
            read(tuberiaMensajes[0], &mensaje, sizeof(mensaje));
            printf("Mensaje recibido: %s\n", mensaje);
        }

    }

    close(tuberia[0]);
    close(tuberia[1]);

    close(tuberiaMensajes[0]);
    close(tuberiaMensajes[1]);

    sem_destroy(semaforo);
    munmap(semaforo, sizeof(sem_t)); // Liberar el área de memoria compartida
    return 0;
}