#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

int main(int argc, char const *argv[]) {

  	int tuberia[2], tuberiaMensajes[2];
    pipe(tuberia);
    pipe(tuberiaMensajes);

    // Crear semaforo
    sem_t semaforo;
    sem_init(&semaforo, 1, 0);

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
        }
        
        if (0) {
            const char *mensaje = "Proceso 3 no iniciado";
            write(tuberiaMensajes[1], &mensaje, sizeof(mensaje));
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
            // Asignar la ruta del ejecutable a una variable llamada prueba
            ruta = argv[1];
        }

		write(tuberia[1], &ruta, sizeof(ruta));
    }

	close(tuberia[0]);
	close(tuberia[1]);

    close(tuberiaMensajes[0]);
    close(tuberiaMensajes[1]);

    sem_destroy(&semaforo);
    return 0;
}