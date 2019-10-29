#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char *argv[])
{
    int num_N,  // number of numbers
        sum = 0,
        partialSum = 0,
        mean,
        i;
    double stdDev;

    if (argc == 2)
    {
        num_N = atoi(argv[1]);
    }

    // allocate numbers array and generate random numbers
    int* numbers = (int *) malloc(num_N * sizeof(int));
    for (i = 0; i < num_N; i++)
    {
        numbers[i] = rand() % 100 + 1;  // generate a number between 0 and 100
        printf("number %d: %d\n", i, numbers[i]);
    }

    // caclulate mean
    for (i = 0; i < num_N; i++)
    {
        sum += numbers[i];
    }
    mean = sum / num_N;
    printf("Sum: %d\n", sum);
    printf("Mean: %d\n", mean);

    // calculate std. dev.
    for (i = 0; i < num_N; i++)
    {
        partialSum += (numbers[i] - mean) * (numbers[i] - mean);
    }
    stdDev = (double) partialSum / (num_N - 1);
    stdDev = sqrt(stdDev);
    printf("stdDev: %f\n", stdDev);

    free(numbers);
    return 0;
}