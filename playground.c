#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

int main() {

	int tuberia[2];
	
	if (pipe(tuberia) < 0) {
		perror("Error en pipe()\n");
		return(1);
	}

	pid_t pid = fork();
 
	if (pid == 0) {
		close(tuberia[1]);
		printf("Hijo\n");
		int datoRecibido;
		read(tuberia[0], &datoRecibido, sizeof(int));
		printf("Dato recibido: %d\n", datoRecibido);


	} else if (pid > 0) {

		close(tuberia[0]);
		printf("Papa\n");
		int datoEnviado = 999;
		printf("Dato enviado: %d\n", datoEnviado);
		write(tuberia[1], &datoEnviado, sizeof(int));
	} 
	close(tuberia[0]);
	close(tuberia[1]);
	return 0;
}
