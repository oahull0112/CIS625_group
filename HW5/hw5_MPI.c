#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <mpi.h>


int num_N_Global;

int main(int argc, char *argv[])
{
    int num_N,  // local number of numbers
        rc,
        numTasks,
        rank,
        remainder,
        i;
    double stdDev,
            mean,
            sum = 0,
            partialSum = 0;
    int* numbers;
    double* receiveBuff;
    MPI_Status status;

    // Init MPI
    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS)
    {
        printf("Error starting MPI.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // check commandline args and then set num_N to given number
    if (argc == 2)
    {
        num_N_Global = atoi(argv[1]);
    }
    num_N = num_N_Global / numTasks;
    
    remainder = num_N_Global % numTasks;
    if (rank == (numTasks - 1))
    {
        num_N += remainder;
    }

    // allocate numbers array and generate random numbers
    receiveBuff = (double *) malloc(numTasks * sizeof(double));
    numbers = (int *) malloc(num_N * sizeof(int));
    for (i = 0; i < num_N; i++)
    {
        numbers[i] = rand() % 100 + 1;  // generate a number between 0 and 100
        printf("Number %d: %d\n", i, numbers[i]);
    }

    // caclulate mean
    for (i = 0; i < num_N; i++)
    {
        sum += numbers[i];
    }
    mean =  (double) sum / num_N;
    printf("Sum %d: %f\n", rank, sum);
    printf("Mean %d: %f\n", rank, mean);

    MPI_Allgather(&mean, 1, MPI_DOUBLE, receiveBuff, 1, MPI_DOUBLE, MPI_COMM_WORLD);

    mean = 0;
    for (i = 0; i < numTasks; i++)
    {
        mean += receiveBuff[i];
    }
    mean = mean / numTasks;

    for (i = 0; i < num_N; i++)
    {
       partialSum += (numbers[i] - mean) * (numbers[i] - mean);
    }

    MPI_Gather(&partialSum, 1, MPI_DOUBLE, receiveBuff, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {  
        sum = 0;
        for (i = 0; i < numTasks; i++)
        {
            sum += receiveBuff[i];
        }
        // calculate std. dev.
        stdDev =  sum / (num_N_Global - 1);
        stdDev = sqrt(stdDev);
        printf("stdDev: %f\n", stdDev);
    }
    
    free(numbers);
    free(receiveBuff);
    MPI_Finalize();
    return 0;
}