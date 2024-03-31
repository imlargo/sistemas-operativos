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

    int tuberia[2], tuberiaMensajes[2];
    pipe(tuberia);
    pipe(tuberiaMensajes);

    // Crear semáforos
    sem_unlink("/semaforoPadre");
    sem_t *semP;
    semP = sem_open("/semaforoPadre", O_CREAT, 0666, 0);

    sem_unlink("/semaforoHijo");
    sem_t *semH;
    semH = sem_open("/semaforoHijo", O_CREAT, 0666, 0);

    pid_t pid = fork();

    if (pid > 0) {
        // Proceso padre
        close(tuberia[0]);
        close(tuberiaMensajes[1]);

        const char *ruta;

        if (argc == 1) {
            char *base = "/bin/uname";
            printf("Uso: p1 %s\n", base);
            ruta = base;
        } else {
            ruta = argv[1];
        }

        write(tuberia[1], &ruta, sizeof(ruta));
        sem_post(semH);

        while (1) {
            sem_wait(semP);

            char mensaje[4096];
            int bytesRead = read(tuberiaMensajes[0], mensaje, sizeof(mensaje) - 1);
            if (bytesRead >= 0) {
                mensaje[bytesRead] = '\0'; // Aseguramos que el buffer sea una cadena válida en C
            }
            if (strcmp(mensaje, "exit") == 0) {
                sem_post(semH);
                break;
            }
            printf("Mensaje recibido: %s", mensaje);
            
            sem_post(semH);
        }

        sem_close(semH);
        sem_close(semP);

        sem_unlink("/semaforoPadre");

    } else if (pid == 0) {
        // Proceso hijo
        
        close(tuberia[1]);
        close(tuberiaMensajes[0]);

        sem_wait(semH);

        char* ruta;
        read(tuberia[0], &ruta, sizeof(ruta));

        if (access(ruta, X_OK) != 0) {
            const char *mensaje = "No se encuentra el archivo a ejecutar\n";
            write(tuberiaMensajes[1], mensaje, strlen(mensaje));
            sem_post(semP);
            sem_wait(semH);

            const char *salidaM = "exit";
            write(tuberiaMensajes[1], salidaM, strlen(salidaM));
            sem_post(semP);
            sem_wait(semH);
            return 0;
        }

        // Intentar conectar con proceso 3
        char *memoriaCompartida;
        int areaMemoriaCompartida = shm_open("/memoriaCompartida", O_RDWR, 0666);
        if (areaMemoriaCompartida == -1) {
            const char *mensaje = "Proceso p3 no parcece estar en ejecución\n";
            write(tuberiaMensajes[1], mensaje, strlen(mensaje));
            sem_post(semP);
            sem_wait(semH);
            
            const char *salidaM = "exit";
            write(tuberiaMensajes[1], salidaM, strlen(salidaM));
            sem_post(semP);
            sem_wait(semH);
            return 0;
        }
	    memoriaCompartida = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, areaMemoriaCompartida, 0);

        sem_t *semaforoPr2;
        semaforoPr2 = sem_open("/semaforoPr2", O_RDWR);
        
        // Escribir la ruta en memoria compartida
        sprintf(memoriaCompartida, "%s", ruta);

        // Descongelar proceso 3
        sem_post(semaforoPr2);

        // Esperar y leer la salida del proceso 3
        sem_wait(semH);

        char buffer[4096];
        strcpy(buffer, memoriaCompartida);
        write(tuberiaMensajes[1], buffer, strlen(buffer));

        // Esperar que el proceso padre termine de imprimir el resultado
        sem_post(semP);
        sem_wait(semH);

        // Salir
        const char *salida = "exit";
        write(tuberiaMensajes[1], salida, strlen(salida));
        sem_post(semP);
        sem_wait(semH);

        // Enviar señal a proceso 3 para eliminar memoria
        sem_post(semaforoPr2);
        sem_close(semaforoPr2);

        sem_close(semH);
        sem_close(semP);
        sem_unlink("/semaforoHijo");

        munmap(memoriaCompartida, 4096);
    }

    close(tuberia[0]);
    close(tuberia[1]);
    close(tuberiaMensajes[0]);
    close(tuberiaMensajes[1]);
    return 0;
}