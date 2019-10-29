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
  int icount = 0;
  int recCounter = 0;
  int hits[100] = {-1};
  int hitBuf[100] = {-1};
  MPI_Status Status;


  rc = MPI_Init(&argc, &argv);
  if ( rc != MPI_SUCCESS){
    printf("Error starting MPI program. Terminating \n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }

  tstart = myclock(); // set the zero time
  tstart = myclock(); // start the clock

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks); // how many tasks?
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // my MPI id

  nwords = atoi(argv[1]);
  nlines = atoi(argv[2]);
 
  // Figure out which task gets which lines:
  line_remainder = nlines % (numtasks-1);
  my_nlines = (nlines/(numtasks-1));
  my_linestart = (rank-1)*my_nlines ;
  my_lineend = my_linestart + my_nlines;

  // Give leftover lines to the last mpi task
  if (rank == numtasks - 1)
  {
    my_nlines = my_nlines + line_remainder;
    my_lineend = my_lineend + line_remainder;
  }

  // task 0 reads in the keywords
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

  // worker tasks read in their pieces of wikilines
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

  // loop over the number of keywords
  for (i = 0 ; i < nwords; i++)
  {
    if (rank == 0) 
    {
      for (j = 1; j < numtasks ; j ++)
      {
        // send the current word to be worked on to all worker tasks:
        MPI_Send(word[i], 10, MPI_CHAR, j, 0, MPI_COMM_WORLD);
      }
        // receive the results from each worker task
      printf("\n");
      printf("%s", word[i]);
      while (recCounter < numtasks-1)
      {
        MPI_Recv(&hitBuf, 100, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Status );
        recCounter++;
        for ( k = 0; k < 99; k++)
        {
          if(hitBuf[k] != -1) // means if there is n hits, print first n entries
          {
            printf(", %d", hitBuf[k]);
          }
        }
      }
      // reset the receive-counter and the hit buffer:
      recCounter = 0;
      for (k = 0; k < 100; k++)
      {
        hitBuf[k] = -1;
      }
    }
    else // worker tasks:
    {
      // receive the current keyword:
      MPI_Recv( &currentWord, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &Status);
      icount = 0;
      for (j = 0; j < 100; j++) // only want the first 100 hits per worker task
      {
        hits[j] = -1;
      }
      for ( k = 0; k < my_nlines ; k++ )
      {
        if (strstr(line[k], currentWord) != NULL && icount < 100 )
          {
           // append to 100-index list
            hits[icount] = k + my_linestart;
            icount++;
          }
      }
      // send the hits index array back to the master task
      MPI_Send(hits, 100, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
  }

  ttotal = myclock() - tstart;

  // print out the timing info
  if (rank == 0)
  {
    printf("\n");
    printf("DATA, 1, %lf\n", ttotal);
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

