#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

int main(int argc, char const *argv[])
{
    int num_N = 20, i;
    int* numbers;
    numbers = (int *) malloc(num_N * sizeof(int));
    srand(time(0));
    for (i = 0; i < num_N; i++)
    {
        numbers[i] = rand() % 100 + 1;  // generate a number between 0 and 100
        printf("Number %d: %d\n", i, numbers[i]);
    }
    return 0;
}