#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <mpi.h>


int num_N_Global;
int num_threads;

int main(int argc, char *argv[])
{
    int num_N,  // local number of numbers
        rc,
        numTasks,
        rank,
        remainder,
        thread_id,
        my_start,
        my_end,
        i;
    double stdDev,
            mean,
            sum = 0,
            partialSum = 0,
            thread_sum = 0;
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
    if (argc == 3)
    {
        num_N_Global = atoi(argv[1]);
        num_threads = atoi(argv[2]);
    }
    num_N = num_N_Global / numTasks;
    omp_set_num_threads(num_threads);
    thread_id = omp_get_thread_num();
    
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
       // printf("Number %d: %d\n", i, numbers[i]);
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
    // note: we can use num_threads here if we want.. just cant put it into the private list
    #pragma omp parallel private(thread_id, my_start, my_end, thread_sum)
    {
      thread_id = omp_get_thread_num();
      my_start = thread_id*(num_N/num_threads);
      my_end = my_start + (num_N/num_threads);
      if(thread_id == num_threads - 1)
      {
        my_end = num_N;
      }
    for (i = my_start; i < my_end; i++)
      {
         thread_sum += (numbers[i] - mean) * (numbers[i] - mean);
      }

#pragma omp critical
    {
      partialSum += thread_sum;
    }
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
