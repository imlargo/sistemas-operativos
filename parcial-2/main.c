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

#include <stdint.h> /* for uint64 definition */
#include <time.h>   /* for clock_gettime */

/*
Escriba un programa en C que realice la traducción de direcciones de memoria en un sistema que
tiene un espacio virtual de direccionamiento de 32 bits con páginas de 4 KiB. El programa debe leer
de manera indefinida y hasta que el usuario pulse la letra «s», una dirección en decimal y mostrar:
(a) el número de página en decimal y en binario, (b) el desplazamiento dentro de la página en
decimal y en binario, (c) el tiempo en segundos que tomó la operación y (d) si la traducción produce
un TLB Hit o un TLB Miss. Para simular el TLB debe usar memoria en el segmento heap del proceso
(obligatorio). Implemente toda la lógica de la traducción usando el TLB como caché, de acuerdo con
los conceptos explicados en clase.
*/

/*
- espacio virtual de direccionamiento de 32 bits
- páginas de 4 KiB.

int sizeBits = 32;          // 2 ^ 32 -> m = 32 -> 4G
int sizeBytesPagina = 4096; // 4096 bytes -> 2 ^ 12 -> n = 12

*/

int valueM = 32;
int valueN = 12;

int NUM_ENTRADAS = 5;

int getSize(int *first, int *last)
{
    if (*first == -1 && *last == -1)
    {
        return 0;
    }

    int a = NUM_ENTRADAS - (*first) + (*last);
    return a % NUM_ENTRADAS + 1;
}

int Enqueue(int *first, int *last)
{
    int size = getSize(first, last);

    if (size == 0)
    {
        *first = 0;
    }

    int index;
    if (size < NUM_ENTRADAS)
    {
        index = (*last + 1) % NUM_ENTRADAS;
        *last = index;
    }
    else
    {
        index = -1;
    }

    return index;
}

int Dequeue(int *first, int *last)
{
    int size = getSize(first, last);

    if (size == 0)
    {
        return -1;
    }

    int index = *first;

    if (size == 1)
    {
        *first = -1;
        *last = -1;
    }
    else
    {
        *first = (*first + 1) % NUM_ENTRADAS;
    }

    return index;
}

char *decimalToBinary(int num)
{
    int INT_SIZE = 32; // sizeof(int) * 8;
    int c, result;

    // Allocate memory for the binary string (+1 for the null terminator)
    char *binary = (char *)malloc((INT_SIZE + 1) * sizeof(char));
    if (binary == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    for (c = INT_SIZE - 1; c >= 0; c--)
    {
        result = num >> c;

        if (result & 1)
        {
            binary[INT_SIZE - 1 - c] = '1';
        }
        else
        {
            binary[INT_SIZE - 1 - c] = '0';
        }
    }

    // Null-terminate the string
    binary[INT_SIZE] = '\0';

    return binary;
}

int binaryToDecimal(char *binary)
{
    int decimal = 0;
    int c;
    int length = 0;

    // Get the length of the string
    while (binary[length] != '\0')
    {
        length++;
    }

    for (c = 0; c < length; c++)
    {
        if (binary[c] == '1')
        {
            decimal = decimal * 2 + 1;
        }
        else if (binary[c] == '0')
        {
            decimal = decimal * 2;
        }
    }

    return decimal;
}

int obtenerNumeroPagina(char *direccionBinario)
{
    // Copiar los bits de la página
    char *paginaBinario = (char *)malloc((valueM - valueN + 1) * sizeof(char));
    strncpy(paginaBinario, direccionBinario, valueM - valueN);
    paginaBinario[valueM - valueN] = '\0'; // Añadir el terminador nulo

    int pagina = binaryToDecimal(paginaBinario);

    free(paginaBinario);

    return pagina;
}

int obtenerDesplazamiento(char *direccionBinario)
{
    // Copiar los bits del desplazamiento
    char *desplazamientoBinario = (char *)malloc((valueN + 1) * sizeof(char));
    strncpy(desplazamientoBinario, direccionBinario + (valueM - valueN), valueN);
    desplazamientoBinario[valueN] = '\0'; // Añadir el terminador nulo

    int desplazamiento = binaryToDecimal(desplazamientoBinario);

    free(desplazamientoBinario);

    return desplazamiento;
}

void logMensaje(int direccion_virtual, int isHit, int pagina, int desplazamiento, int direccion_reemplazo, double tiempo)
{

    /*
        TLB Miss
        Página: 4
        Desplazamiento: 3602
        Página en binario: 00000000000000000100
        Desplazamiento en binario: 111000010010
        Politica de reemplazo: 0x0
        Tiempo: 0.000049 segundos
    */

    if (isHit == 1)
    {
        printf("TLB Hit\n");
    }
    else
    {
        printf("TLB Miss\n");
    }

    printf("Página: %d\n", pagina);

    printf("Desplazamiento: %d\n", desplazamiento);

    char *paginaBinario = decimalToBinary(pagina);
    char *desplazamientoBinario = decimalToBinary(desplazamiento);

    printf("Página en binario: %s\n", paginaBinario);
    printf("Desplazamiento en binario: %s\n", desplazamientoBinario);

    if (direccion_reemplazo == -1)
    {
        printf("Politica de reemplazo: 0x0\n");
    }
    else
    {
        printf("Politica de reemplazo: %d\n", direccion_reemplazo);
    }

    printf("Tiempo: %.6f segundos\n", tiempo);

    free(paginaBinario);
    free(desplazamientoBinario);

    printf("\n");
}

void iniciarContador(struct timespec *start)
{
    clock_gettime(CLOCK_MONOTONIC, start);
}

void finalizarContador(struct timespec *end)
{
    clock_gettime(CLOCK_MONOTONIC, end);
}

double calcularDuracion(struct timespec *start, struct timespec *end)
{

    long seconds = end->tv_sec - start->tv_sec;
    long nanoseconds = end->tv_nsec - start->tv_nsec;
    double elapsed = seconds + nanoseconds * 1e-9;

    return elapsed;
}

int tlbHas(int direccion, int *entry1, int *entry2, int *entry3, int *entry4, int *entry5)
{
    if (entry1 != NULL && *entry1 == direccion)
    {
        return 1;
    }

    if (entry2 != NULL && *entry2 == direccion)
    {
        return 1;
    }

    if (entry3 != NULL && *entry3 == direccion)
    {
        return 1;
    }

    if (entry4 != NULL && *entry4 == direccion)
    {
        return 1;
    }

    if (entry5 != NULL && *entry5 == direccion)
    {
        return 1;
    }

    return 0;
}

int removeTlb(int *firstEntry, int *lastEntry, int *entry1, int *entry2, int *entry3, int *entry4, int *entry5)
{
    int index = Dequeue(firstEntry, lastEntry);
    int direccionEliminada = -1;

    switch (index)
    {
    case 0:
        direccionEliminada = *entry1;
        *entry1 = 0;
        return direccionEliminada;
        break;

    case 1:
        direccionEliminada = *entry2;
        *entry2 = 0;
        return direccionEliminada;
        break;

    case 2:
        direccionEliminada = *entry3;
        *entry3 = 0;
        return direccionEliminada;
        break;

    case 3:
        direccionEliminada = *entry4;
        *entry4 = 0;
        return direccionEliminada;
        break;

    case 4:
        direccionEliminada = *entry5;
        *entry5 = 0;
        return direccionEliminada;
        break;

    default:
        printf("Error\n");
        break;
    }

    return direccionEliminada;
}

int saveInTlb(int direccion, int *firstEntry, int *lastEntry, int *entry1, int *entry2, int *entry3, int *entry4, int *entry5)
{

    int index = Enqueue(firstEntry, lastEntry);

    switch (index)
    {
    case 0:
        *entry1 = direccion;
        break;

    case 1:
        *entry2 = direccion;
        break;

    case 2:
        *entry3 = direccion;
        break;

    case 3:
        *entry4 = direccion;
        break;

    case 4:
        *entry5 = direccion;
        break;

    default:
        printf("Error\n");
        break;
    }
}

int main()
{

    struct timespec start, end;

    int entry1 = 0;
    int entry2 = 0;
    int entry3 = 0;
    int entry4 = 0;
    int entry5 = 0;

    int firstEntry = -1;
    int lastEntry = -1;

    while (1)
    {

        char *input = malloc(100 * sizeof(char));
        printf("Ingrese dirección virtual: ");
        scanf("%99s", input);

        if (input[0] == 's')
        {
            printf("Saliendo...\n");
            free(input);
            break;
        }

        int direccion_virtual = atoi(input);
        // TLB desde 0x00401251 hasta 0x004014D6

        int isHit = tlbHas(direccion_virtual, &entry1, &entry2, &entry3, &entry4, &entry5);
        int direccion_reemplazo = -1;

        if (isHit == 0)
        {
            int tlbFull = getSize(&firstEntry, &lastEntry) == NUM_ENTRADAS;
            if (tlbFull)
            {
                direccion_reemplazo = removeTlb(&firstEntry, &lastEntry, &entry1, &entry2, &entry3, &entry4, &entry5);
            }

            saveInTlb(direccion_virtual, &firstEntry, &lastEntry, &entry1, &entry2, &entry3, &entry4, &entry5);
        }

        int sizeTlb = getSize(&firstEntry, &lastEntry);

        iniciarContador(&start);
        char *direccionBinario = decimalToBinary(direccion_virtual);
        int pagina = obtenerNumeroPagina(direccionBinario);
        int desplazamiento = obtenerDesplazamiento(direccionBinario);

        finalizarContador(&end);
        double tiempo = calcularDuracion(&start, &end);

        logMensaje(direccion_virtual, isHit, pagina, desplazamiento, direccion_reemplazo, tiempo);

        free(direccionBinario);
        free(input);
    }

    return 0;
}
