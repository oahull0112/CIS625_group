#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "sys/types.h"
#include "sys/sysinfo.h"

#define word_length 10

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
 // char *buf;
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

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks); // how many tasks?
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // my MPI id

  nwords = atoi(argv[1]);
  nlines = atoi(argv[2]);
 
  line_remainder = nlines % (numtasks-1);
  my_nlines = (nlines/(numtasks-1));
  my_linestart = (rank-1)*my_nlines ;
  my_lineend = my_linestart + my_nlines;

  if (rank == numtasks - 1)
  {
    my_nlines = my_nlines + line_remainder;
    my_lineend = my_lineend + line_remainder;
  }

  printf("my rank: %d, my start: %d, my end: %d, my nlines: %d \n", rank, my_linestart, my_lineend, my_nlines);

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

  for (i = 0 ; i < nwords; i++)
  {
    if (rank == 0) 
    {
      for (j = 1; j < numtasks ; j ++)
      {
        // send the current word to be worked on:
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
      recCounter = 0;
      for (k = 0; k < 100; k++)
      {
        hitBuf[k] = -1;
      }
    }
    else
    {
      MPI_Recv( &currentWord, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &Status);
      icount = 0;
      for (j = 0; j < 100; j++)
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
      MPI_Send(hits, 100, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
//      receive the number of hits, allocate a structure to receive the index array
//        - possible structure is an array with number of entries corresponding to number of worker tasks
//        - and each entry points to an array allocated with the hit size received from the worker
//      receive the list of indices back from each worker
//      once all indices have been received, dump out to the display
// // do a for-loop here with MPI broadcast?
  }
  
  MPI_Finalize();
  return 0;
} // end of main
