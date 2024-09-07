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

#define BILLION 1000000000L

char *decimalToBinary(int num)
{
    int INT_SIZE = 8; // sizeof(int) * 8;
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


void logMensaje(int direccion_virtual, int esMiss, int pagina, int desplazamiento, int direccion_reemplazo, float tiempo)
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

    if (esMiss)
    {
        printf("TLB Miss\n");
    }
    else
    {
        printf("TLB Hit\n");
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

    printf("Tiempo: %f segundos\n", tiempo);

    free(paginaBinario);
    free(desplazamientoBinario);

    printf("\n");
}

int main()
{

    int *entry1 = NULL;
    int *entry2 = NULL;
    int *entry3 = NULL;
    int *entry4 = NULL;
    int *entry5 = NULL;

    while (1)
    {
    
        char *input;
        printf("Ingrese dirección virtual: ");
        scanf("%s", input);

        if (input[0] == 's')
        {
            printf("Saliendo...\n");
            break;
        }

        int direccion_virtual = atoi(input);

        // Ingrese dirección virtual: 19986
        // TLB desde 0x00401251 hasta 0x004014D6

        float tiempo;
        int esMiss, pagina, desplazamiento, direccion_reemplazo;

        esMiss = 1;
        pagina = 4;
        desplazamiento = 3602;
        direccion_reemplazo = -1;
        tiempo = 0.000049;

        logMensaje(direccion_virtual, esMiss, pagina, desplazamiento, direccion_reemplazo, tiempo);


    }

    return 0;
}

