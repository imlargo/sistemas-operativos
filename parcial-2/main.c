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
int ENTRY_SIZE = (3 * sizeof(int)) + ((20 + 1) * sizeof(char)) + ((12 + 1) * sizeof(char));

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

void logMensaje(int direccion_virtual, char* TLB, int isTlbHit, int paginaDecimal, int desplazamientoDecimal, char* paginaBinario, char* desplazamientoBinario, char* direccion_reemplazo, double tiempo)
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

   // TLB desde 0x00401251 hasta 0x004014D6
    printf("TLB desde %p hasta %p\n", &TLB[0], &TLB[0] + (ENTRY_SIZE * 5));

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

    if (direccion_reemplazo == NULL)
    {
        printf("Politica de reemplazo: 0x0\n");
    }
    else
    {
        printf("Politica de reemplazo: %p\n", direccion_reemplazo);
    }

    printf("Tiempo: %.6f segundos\n", tiempo);

    printf("\n");
}


/*
    Funciones del TLB
*/

int getEntryStartAdress(int index) {
    return index * ENTRY_SIZE;
}

char* getEntry(int index, char* TLB) {
    int entryStartAdress = getEntryStartAdress(index);
    char *entry = TLB + entryStartAdress;
    return entry;
}

int getDireccionEntry(char *entry) {
    return *((int*)entry);
}

char* getEntryToDequeue(int *firstEntry, int *lastEntry, char *TLB)
{
    int indexEntry = Dequeue(firstEntry, lastEntry);
    char *entry = getEntry(indexEntry, TLB);
    return entry;
}

char* getEntryToEnqueue(int *firstEntry, int *lastEntry, char* TLB)
{
    int indexEntry = Enqueue(firstEntry, lastEntry);
    char *entry = getEntry(indexEntry, TLB);
    return entry;
}

char* deleteTlbEntry(char* TLB, char *entry)
{
    *((int*)entry) = -1;
    return entry;
}

void getDataFromEntry(char *entry, int *ptrPaginaDecimal, int *ptrDesplazamientoDecimal, char *paginaBinario, char *desplazamientoBinario)
{
    *ptrPaginaDecimal = *((int*)(entry + 4));
    *ptrDesplazamientoDecimal = *((int*)(entry + 8));

    strncpy(paginaBinario, entry + 12, 20 + 1);
    strncpy(desplazamientoBinario, entry + 32 + 1, 12 + 1);
}

void saveDataInEntry(char* entry, int direccion, int pagina, int desplazamiento, char *paginaBinario, char *desplazamientoBinario) {
    
    *((int*)entry) = direccion;
    *((int*)(entry + 4)) = pagina;
    *((int*)(entry + 8)) = desplazamiento;

    strncpy(entry + 12, paginaBinario, 32 + 1);
    strncpy(entry + 32 + 1, desplazamientoBinario, 12 + 1);
}

char* TlbFind(char* TLB, int direccion) {

    for (int i = 0; i < 5; i++)
    {
        char *entry = getEntry(i, TLB);
        int entryDir = *((int*)entry);

        if (entryDir == direccion)
        {
            return entry;
        }
    }

    return NULL;
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
        char* direccion_reemplazo = NULL;
        int paginaDecimal;
        int desplazamientoDecimal;
        char *paginaBinario;
        char *desplazamientoBinario;
        
        char *tlbHitEntry = TlbFind(TLB, direccion_virtual);

        // Si es tlb miss, agregar a la cola
        if (tlbHitEntry == NULL)
        {   
            isTlbHit = 0;
            // Si el tlb esta lleno, se saca de la cola un elemento
            if (getSize(&firstEntry, &lastEntry) == NUM_ENTRADAS)
            {   
                char *entryToDequeue  = getEntryToDequeue(&firstEntry, &lastEntry, TLB);
                direccion_reemplazo = deleteTlbEntry(TLB, entryToDequeue);
            }

            paginaBinario = calcularNumeroPaginaEnBinario(direccionBinario);
            desplazamientoBinario = calcularDesplazamientoEnBinario(direccionBinario);
            paginaDecimal = binaryToDecimal(paginaBinario);
            desplazamientoDecimal = binaryToDecimal(desplazamientoBinario);
            
            char *entryToEnqueue = getEntryToEnqueue(&firstEntry, &lastEntry, TLB);
            saveDataInEntry(entryToEnqueue, direccion_virtual, paginaDecimal, desplazamientoDecimal, paginaBinario, desplazamientoBinario);

        } else {
            isTlbHit = 1;

            getDataFromEntry(tlbHitEntry, &paginaDecimal, &desplazamientoDecimal, paginaBinario, desplazamientoBinario);
        }

        // Finalizar contador e imprimir mensaje
        finalizarContador(&endTime);
        logMensaje(
            direccion_virtual, 
            TLB,
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
