# sistemas-operativos
....

SISTEMAS OPERATIVOS 2024-1S
EJERCICIO DE IPC Y SINCRONIZACIÓN DE PROCESOS

Descripción del problema de comunicación y sincronización de procesos en Linux. La solución
debe implementarse en lenguaje C.
- Proceso No. 1: recibe como parámetro desde la línea de comandos, la orden de ejecutar
un comando (no interactivo y sin argumentos) disponible en el sistema operativos Linux (p.
ej.: ls, date, uname, ps). Este proceso debe validar que se le haya pasado un parámetro
desde la línea de comandos, en caso de ejecutarse sin parámetros, debe indicar un
mensaje como: «Uso: p1 /ruta/al/ejecutable». El comando que se pasa como
parámetro debe pasarse con la ruta completa del ejecutable. Por ejemplo, el comando ls
hay que pasarlo al proceso No. 1 como /usr/bin/ls. Para conocer la ruta completa de
un comando se puede usar el comando which seguido del nombre del comando, por
ejemplo: which ls retorna con /usr/bin/ls. Consulte man which.

- Proceso No. 2: recibe por una tubería sin nombre el comando que se entregó al proceso
No. 1 y verifica si el archivo ejecutable asociado al comando recibido existe en el sistema
de archivos. En caso de que no exista, debe responder por una tubería sin nombre al
proceso No. 1 el mensaje «No se encuentra el archivo a ejecutar». También debe
verificar que el proceso No. 3 esté en ejecución, si el proceso No. 3 no está en ejecución,
responde por la misma tubería sin nombre que tiene con el proceso No. 1, el mensaje:
«Proceso p3 no parece estar en ejecución». El proceso No. 1 cuando reciba del
proceso No. 2 cualquier mensaje por la tubería entre ambos procesos, lo muestra por la
salida estándar (pantalla) y ambos procesos terminan su ejecución.

- Proceso No. 3: es un proceso autónomo, independientemente creado, se ejecuta de
primero (en una segunda terminal) y recibe por un área de memoria compartida con el
proceso No. 2, el comando verificado por este último y ejecuta el comando creando una
tubería sin nombre para la redirección de la salida estándar del proceso asociado al
comando. El resultado de la ejecución del comando (la salida del comando ejecutado) NO
se debe mostrar por la salida estándar en el proceso No. 3; se debe devolver al área de
memoria compartida con el proceso No. 2, este último pasa por la tubería sin nombre el
resultado de la ejecución del comando al proceso No. 1 y este último es quién muestra el
resultado de la ejecución del comando en la salida estándar.

Requisitos de implementación.
1. Se deben usar la llamadas al sistema fork() y alguna función de la familia de funciones
exec() que resuelva la problemática planteada.

2. Se debe usar el mecanismo de semáforos para resolver problemas de sincronización entre
los procesos involucrados en donde sea necesario. Las llamadas al sistema que se
permiten para el manejo de semáforos son todas aquellas relacionadas a los semáforos
con nombre del estándar POSIX: sem_open(), sem_close(), sem_wait(), sem_post(),
sem_unlink(), sem_getvalue().

3. El área de memoria compartida debe crearse usando las llamadas al sistema para gestión
de memoria compartida del estándar POSIX y de la librería estándar de C: shm_open(),
mmap(), ftruncate(), munmap(), open(), close().

4. Las tuberías sin nombre deben crearse usando las llamadas del estándar POSIX y de la
librería estándar de C: pipe(), open(), close().

5. La redirección de la salida estándar al ejecutar el comando desde el proceso No. 3 debe
gestionarse exclusivamente con la llamada a dup2().

6. Recursos como los archivos, extremos de las tuberías, los semáforos, la memoria
compartida, entre otros, que hayan sido solicitado por los procesos que están bajo su
control, deben liberarse adecuadamente cuando no sean necesarios, bajo condiciones de
error y antes de la terminación normal de los procesos.
Continua en la siguiente página…

7. Cualquier llamada a funciones de la librería estándar de C o del sistema, debe verificarse
el valor retornado, ya que en C no existen instrucciones para el control de excepciones. En
caso de error, cualquier mensaje de error que usted desee escribir, debe escribirse al error
estándar usando perror() y retornar inmediatamente al sistema operativo un valor
diferente de cero.

8. El proceso No.3 es un proceso autónomo, que se debe ejecutar primero, en una terminal
independiente y debe quedar a la espera de lo que le entreguen en el área de memoria
compartida. Los procesos No. 1 y No. 2 se ejecutan en una terminal independiente ya que
son procesos interrelacionados.

9. Si se ejecuta el Proceso No. 1 antes que el proceso No. 3, se debe garantizar que no se
queda bloqueado. El proceso No. 1 debe responder con el mensaje indicando que detectó
que el proceso No. 3 no está en ejecución.

10. El único proceso que escribe datos por la salida estándar es el proceso No. 1 bajo las
condiciones indicadas: a) ejecutable no existe en el sistema de archivos, b) el proceso No.
3 no está en ejecución y c) resultado de la ejecución del comando. Cualquier situación de
error se escribe al error estándar o a la salida estándar por parte de cada proceso que
detecta la condición de error.

Entregables, sustentación y condiciones de calificación.

1. Se deben entregar dos archivos de código escritos en lenguaje C. Un archivo de código
asociado a los procesos No. 1 y No. 2 y un archivo de código asociado al proceso No. 3.