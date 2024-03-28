#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>

int main(int argc, char const *argv[]) {

	// Crear semaforo
	sem_t semaforo;
	sem_init(&semaforo, 1, 1);

	pid_t pid = fork();

	if (pid == 0) {
		// Proceso hijo
	} else {
		// Proceso padre
	}

	sem_destroy(&semaforo);
	return 0;
}