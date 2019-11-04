#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <mpi.h>

// total number of numbers
int num_N_Global;
// total number of omp threads
int numThreads;

int main(int argc, char *argv[])
{
    int num_N,
        rc,
        numTasks,
        rank,
        myThreadID,
        myStart,            
        myEnd,              
        remainder,
        i;
    double stdDev,
           mean,
           sum = 0,
           partialSum = 0,
           myPartialSum = 0;
    int* numbers;
    double* receiveBuff;
    MPI_Status status;

    // check commandline args, exit if wrong
    if (argc == 3)
    {
        num_N_Global = atoi(argv[1]);
        numThreads = atoi(argv[2]);
    }
    else
    {
        printf("Exiting.\n   Usage: ./hw5 <number of numbers>  <number of omp threads>\n");
        exit(0);
    }

    // Init MPI and OpenMP
    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS)
    {
        printf("Error starting MPI.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    omp_set_num_threads(numThreads);

    // set local tasks num_N and then calc and add remainder to last task
    num_N = num_N_Global / numTasks;
    remainder = num_N_Global % numTasks;
    if (rank == (numTasks - 1))
    {
        num_N += remainder;
    }

    // allocate numbers array and generate random numbers
    receiveBuff = (double *) malloc(numTasks * sizeof(double));
    numbers = (int *) malloc(num_N * sizeof(int));
    srand(time(0) + rank);
    for (i = 0; i < num_N; i++)
    {
        numbers[i] = rand() % 100 + 1;  // generate a number between 0 and 100
        printf("Number %d: %d\n", i, numbers[i]);
    }

    // each task calculates its mean
    for (i = 0; i < num_N; i++)
    {
        sum += numbers[i];
    }
    mean =  (double) sum / num_N;
    printf("Sum %d: %0.0f\n", rank, sum);
    printf("Mean %d: %0.2f\n", rank, mean);

    // each task gathers every tasks mean
    MPI_Allgather(&mean, 1, MPI_DOUBLE, receiveBuff, 1, MPI_DOUBLE, MPI_COMM_WORLD);

    // each task calculates the true mean
    mean = 0;
    for (i = 0; i < numTasks; i++)
    {
        mean += receiveBuff[i];
    }
    mean = mean / numTasks;

    #pragma omp parallel private(myThreadID, myStart, myEnd, myPartialSum)
    {
        myThreadID = omp_get_thread_num();
        myStart = (num_N * myThreadID / numThreads);
        myEnd = myStart + (num_N / numThreads);
        if (myThreadID == (numThreads - 1))
        {
            myEnd = num_N;
        }

        printf("num_N: %d, numThreads: %d\n", num_N, numThreads);
        printf("my MPI Task rank: %d  my OMP thread rank: %d my start: %d my end %d\n", rank, myThreadID, myStart, myEnd);

        for (i = myStart; i < myEnd; i++)
        {
            myPartialSum += (numbers[i] - mean) * (numbers[i] - mean);
        }
        #pragma omp critical
        {
            partialSum += myPartialSum;
        } 
    }

    // each task sends its partial sum to task 0
    MPI_Gather(&partialSum, 1, MPI_DOUBLE, receiveBuff, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // task 0 calculates the total std dev
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
        printf("stdDev: %0.2f\n", stdDev);
    }
    
    free(numbers);
    free(receiveBuff);
    MPI_Finalize();
    return 0;
}