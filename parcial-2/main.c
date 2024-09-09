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

# --- Constantes globales --- #

- espacio virtual de direccionamiento de 32 bits
- páginas de 4 KiB.

2 ^ 32 -> m = 32 bits -> 4G total
4096 bytes por pagina -> 2 ^ 12 -> n = 12
*/


/*
    Funciones de contador
*/
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


int valueM = 32;
int valueN = 12;
int NUM_ENTRADAS = 5; // Cantidad de entradas en el TLB
int ENTRY_SIZE = (3 * sizeof(int)) + (32 * sizeof(char));

/*
    Mecanismo de cola circular
*/

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


/*
    Funciones de utilidad
*/

char *decimalToBinary(int num)
{
    int INT_SIZE = 32;
    int ind, result;

    // Allocate memory for the binary string (+1 for the null terminator)
    char *binario = (char *)malloc((INT_SIZE + 1) * sizeof(char));
    if (binario == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }

    for (ind = INT_SIZE - 1; ind >= 0; ind--)
    {
        result = num >> ind;

        if (result & 1)
        {
            binario[INT_SIZE - 1 - ind] = '1';
        }
        else
        {
            binario[INT_SIZE - 1 - ind] = '0';
        }
    }

    // Null-terminate the string
    binario[INT_SIZE] = '\0';

    return binario;
}

int binaryToDecimal(char *binary)
{
    int decimal = 0;

    // Get the length of the string
    int length = 0;
    while (binary[length] != '\0')
    {
        length++;
    }

    int ind;
    for (ind = 0; ind < length; ind++)
    {
        if (binary[ind] == '1')
        {
            decimal = decimal * 2 + 1;
        }
        else if (binary[ind] == '0')
        {
            decimal = decimal * 2;
        }
    }

    return decimal;
}

char* calcularNumeroPaginaEnBinario(char *direccionBinario)
{
    // Copiar los bits de la página
    char *paginaBinario = (char *)malloc((valueM - valueN + 1) * sizeof(char));
    strncpy(paginaBinario, direccionBinario, valueM - valueN);
    paginaBinario[valueM - valueN] = '\0'; // Añadir el terminador nulo

    return paginaBinario;
}

char* calcularDesplazamientoEnBinario(char *direccionBinario)
{
    // Copiar los bits del desplazamiento
    char *desplazamientoBinario = (char *)malloc((valueN + 1) * sizeof(char));
    strncpy(desplazamientoBinario, direccionBinario + (valueM - valueN), valueN);
    desplazamientoBinario[valueN] = '\0'; // Añadir el terminador nulo

    return desplazamientoBinario;
}

void logMensaje(int direccion_virtual, int isTlbHit, int paginaDecimal, int desplazamientoDecimal, char* paginaBinario, char* desplazamientoBinario, int direccion_reemplazo, double tiempo)
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

    if (isTlbHit == 1)
    {
        printf("TLB Hit\n");
    }
    else
    {
        printf("TLB Miss\n");
    }

    printf("Página: %d\n", paginaDecimal);
    printf("Desplazamiento: %d\n", desplazamientoDecimal);
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

    printf("\n");
}


/*
    Funciones del TLB
*/
int* tlbHas(int direccion, int *entry1, int *entry2, int *entry3, int *entry4, int *entry5)
{
    if (entry1 != NULL && *entry1 == direccion) {
        return entry1;
    }
    if (entry2 != NULL && *entry2 == direccion) {
        return entry2;
    }
    if (entry3 != NULL && *entry3 == direccion) {
        return entry3;
    }
    if (entry4 != NULL && *entry4 == direccion) {
        return entry4;
    }
    if (entry5 != NULL && *entry5 == direccion) {
        return entry5;
    }

    return NULL;
}

int* getEntryToDequeue(int *firstEntry, int *lastEntry, int *entry1, int *entry2, int *entry3, int *entry4, int *entry5)
{
    int indexEntry = Dequeue(firstEntry, lastEntry);

    if (indexEntry == 0) {
        return entry1;
    } else if (indexEntry == 1) {
        return entry2;
    } else if (indexEntry == 2) {
        return entry3;
    } else if (indexEntry == 3) {
        return entry4;
    } else if (indexEntry == 4) {
        return entry5;
    }

    return NULL;
}

int* getEntryToEnqueue(int direccion, int *firstEntry, int *lastEntry, int *entry1, int *entry2, int *entry3, int *entry4, int *entry5)
{
    int indexEntry = Enqueue(firstEntry, lastEntry);

    if (indexEntry == 0) {
        return entry1;
    } else if (indexEntry == 1) {
        return entry2;
    } else if (indexEntry == 2) {
        return entry3;
    } else if (indexEntry == 3) {
        return entry4;
    } else if (indexEntry == 4) {
        return entry5;
    }

    return NULL;
}

void guardarDireccionTlb(int direccion, int *entry)
{
    *entry = direccion;
}

int eliminarDireccionTlb(int direccion, int *entry)
{
    int direccionEliminada = *entry;
    *entry = 0;
    
    return direccionEliminada;
}

void getDataFromEntry(int *entry, int *ptrPaginaDecimal, int *ptrDesplazamientoDecimal, char *paginaBinario, char *desplazamientoBinario)
{
    int direccion = *entry;

    char *dirBin = decimalToBinary(direccion);
    paginaBinario = calcularNumeroPaginaEnBinario(dirBin);
    desplazamientoBinario = calcularDesplazamientoEnBinario(dirBin);
    
    *ptrPaginaDecimal = binaryToDecimal(paginaBinario);
    *ptrDesplazamientoDecimal = binaryToDecimal(desplazamientoBinario);
    
    free(dirBin);
}


/*
    Programa principal
*/
int main()
{


   /*
    dirección de memoria en decimal -> 4bytes
    número de página en decimal -> 4bytes -> 8
    desplazamiento en decimal -> 4bytes -> 12
    número de página en binario -> valueM - valueN -> 20 bytes -> 32
    desplazamiento en binario. -> valueN -> 12 bytes -> 44
   */

    int tlbSize = ENTRY_SIZE * 5;
    char *TLB = malloc(tlbSize);

    struct timespec startTime, endTime;

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
        
        if (strcmp(input, "s") == 0)
        {
            printf("Good bye! ;)\n");
            free(input);
            free(TLB);
            break;
        }

        int direccion_virtual = atoi(input);
        iniciarContador(&startTime);

        char *direccionBinario = decimalToBinary(direccion_virtual);
        int isTlbHit = 0;
        int direccion_reemplazo = -1;
        int paginaDecimal;
        int desplazamientoDecimal;
        char *paginaBinario;
        char *desplazamientoBinario;
        
        // TLB desde 0x00401251 hasta 0x004014D6
        printf("TLB desde %p hasta %p\n", &TLB[0], &TLB[0] + tlbSize);
        int *tlbHitEntry = tlbHas(direccion_virtual, &entry1, &entry2, &entry3, &entry4, &entry5);

        // Si es tlb miss, agregar a la cola
        if (tlbHitEntry == NULL)
        {   
            isTlbHit = 0;
            // Si el tlb esta lleno, se saca de la cola un elemento
            if (getSize(&firstEntry, &lastEntry) == NUM_ENTRADAS)
            {   
                int *entryToDequeue  = getEntryToDequeue(&firstEntry, &lastEntry, &entry1, &entry2, &entry3, &entry4, &entry5);
                direccion_reemplazo = eliminarDireccionTlb(direccion_virtual, entryToDequeue);
            }

            paginaBinario = calcularNumeroPaginaEnBinario(direccionBinario);
            desplazamientoBinario = calcularDesplazamientoEnBinario(direccionBinario);
            paginaDecimal = binaryToDecimal(paginaBinario);
            desplazamientoDecimal = binaryToDecimal(desplazamientoBinario);
            
            int *entryToEnqueue = getEntryToEnqueue(direccion_virtual, &firstEntry, &lastEntry, &entry1, &entry2, &entry3, &entry4, &entry5);
            guardarDireccionTlb(direccion_virtual, entryToEnqueue);

        } else {
            isTlbHit = 1;

            getDataFromEntry(tlbHitEntry, &paginaDecimal, &desplazamientoDecimal, paginaBinario, desplazamientoBinario);
        }

        // Finalizar contador e imprimir mensaje
        finalizarContador(&endTime);
        logMensaje(
            direccion_virtual, 
            isTlbHit, 
            paginaDecimal, 
            desplazamientoDecimal, 
            paginaBinario, 
            desplazamientoBinario, 
            direccion_reemplazo, 
            calcularDuracion(&startTime, &endTime)
        );

        // Liberar recursos
        free(direccionBinario);
        free(paginaBinario);
        free(desplazamientoBinario);
        free(input);
    }

    return 0;
}
