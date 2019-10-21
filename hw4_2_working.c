#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "sys/types.h"
#include "sys/sysinfo.h"

#define word_length 10

double myclock();

int main(int argc, char* argv[])
{
  int nwords;
  int nlines;
  int inlines, inwords; // index of nlines, index of nwords
  int i, j, k, n, err, *count, *totcount;
  double nchars = 0;
  double tstart, ttotal;
  FILE *fd;
  char **word, **line;
  int numtasks, rank, rc;
  int line_remainder, my_nlines, my_lineend, my_linestart;
  int imylines;
  char buf[2001];
  char currentWord[10];
  // we may need to change hitBuf so that task 0 allocates like below
  // but for the worker tasks, may need to malloc?
  // Think about how stuff is getting read into here...
  int hitBuf[100] = {-1};
  int icount = 0;
  int currentCount = 0;
  int lasttask;
  int taskToSendTo;
  int taskToReceiveFrom;
  int n_master_rec = 0;
  int n_receives = 0;
  MPI_Status Status;

  rc = MPI_Init(&argc, &argv);
  if ( rc != MPI_SUCCESS){
    printf("Error starting MPI program. Terminating \n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }

  tstart = myclock();
  tstart = myclock();

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks); // how many tasks?
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // my MPI id

  nwords = atoi(argv[1]);
  nlines = atoi(argv[2]);
  lasttask = numtasks - 1;
 
  // figure out which chunk of wiki lines belong to which task:
  line_remainder = nlines % (numtasks-1);
  my_nlines = (nlines/(numtasks-1));
  my_linestart = (rank-1)*my_nlines ;
  my_lineend = my_linestart + my_nlines;

  if (rank == numtasks - 1)
  {
    my_nlines = my_nlines + line_remainder;
    my_lineend = my_lineend + line_remainder;
  }


  // task 0 reads in all the keywords:
  if(rank == 0)
  {
    word = (char **) malloc( nwords *(1+ sizeof( char * )) );
    for( i = 0; i < nwords; i++ ) 
    {
       word[i] = malloc( word_length );
    }
    fd = fopen("/homes/dan/625/keywords.txt", "r");
    inwords = -1;
    do {
       err = fscanf( fd, "%[^\n]\n", word[++inwords] );
    } while( err != EOF && inwords < nwords );
    fclose( fd );
  } 

  // worker tasks read in their designated wiki lines:
  if (rank != 0)
  {
    line = (char **) malloc( my_nlines * sizeof( char * ) );
    for( i = 0; i < my_nlines; i++ ) 
    {
      line[i] = malloc( 2001 );
    }
    fd = fopen("/homes/dan/625/wiki_dump.txt", "r");
    imylines = 0;
    inlines = 0;
    for (inlines = 0 ; inlines < nlines ; inlines++)
    {
      err = fscanf(fd, "%[^\n]\n", buf );
      if ( buf != NULL)
      {
        if (inlines >= my_linestart && inlines < my_lineend)
        {
          strcpy(line[imylines], buf);
          imylines++;
        }
      }
    }
    fclose(fd);
  }

  if (rank == 0) 
  {
    for (i = 0 ; i < nwords; i++)
    {
      // In this case, have task 0 do no actual work (for now?).
      // send the current word to be worked on:
      for (k=0; k<100; k++)
      {
        hitBuf[k]=-1;
      }
      currentCount = 0;
      MPI_Send(word[i], 10, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
      MPI_Send(&currentCount, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
      MPI_Send(&hitBuf, 100, MPI_INT, 1, 0, MPI_COMM_WORLD);
    //  for (k=0; k<100; k++)
    //  {
    //    hitBuf[k]=-1;
    //  }
    }
  }
  else
  {
    // could change source here to rank-1
    // try this later
    while (n_receives < nwords)
    {
      // receive current word and its current hit count from the previous task
      taskToReceiveFrom = rank - 1;
      MPI_Recv( &currentWord, 10, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Status);
      MPI_Recv( &currentCount, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Status);
      MPI_Recv(&hitBuf, 100, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Status);
      for ( k = 0; k < my_nlines ; k++ )
      {
        // need to modify this part so that it also appends the index of the line:
        if (strstr(line[k], currentWord) != NULL && currentCount < 100 )
          {
            hitBuf[currentCount] = k + my_linestart;
            currentCount++;
          }
      }
      taskToSendTo = (rank+1)%numtasks; // "wraps around" so the last task sends to task 0
      MPI_Send(&currentWord, 10, MPI_CHAR, taskToSendTo, 0, MPI_COMM_WORLD);
      MPI_Send(&currentCount, 1, MPI_INT, taskToSendTo, 0, MPI_COMM_WORLD);
      MPI_Send(&hitBuf, 100, MPI_INT, taskToSendTo, 0, MPI_COMM_WORLD);
      n_receives++;
    }
  }
 
  if (rank == 0)
  {
    // once task 0 sends all words out, can start receiving words back:
    // Do we print out word : total ONLY if total != 0, or do we print out the word but not the total if total = 0?
    while (n_receives < nwords)
    {
       MPI_Recv(&currentWord, 10, MPI_CHAR, lasttask, 0, MPI_COMM_WORLD, &Status);
       MPI_Recv(&currentCount, 1, MPI_INT, lasttask, 0, MPI_COMM_WORLD, &Status );
       MPI_Recv(&hitBuf, 100, MPI_INT, lasttask, 0, MPI_COMM_WORLD, &Status);
       printf("\n %s: count %d", currentWord, currentCount);
       for (k=0; k<99; k++) // NOTE: I think this should be k < 100...
       {
         if(hitBuf[k] != -1)
         {
           printf(", %d", hitBuf[k]);
         }
       }
       n_receives++;
    }
  }

  ttotal = myclock() - tstart;
  if (rank == 0)
  {
    printf("\n");
    printf("DATA, 2, %lf\n", ttotal);
  }

  MPI_Finalize();
  return 0;
} // end of main

double myclock() {
  static time_t t_start = 0;  // Save and subtract off each time
  struct timespec ts; 

  clock_gettime(CLOCK_REALTIME, &ts);
  if( t_start == 0 ) t_start = ts.tv_sec;

  return (double) (ts.tv_sec - t_start) + ts.tv_nsec * 1.0e-9;
}

