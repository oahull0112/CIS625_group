#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "sys/types.h"
#include "sys/sysinfo.h"

#define word_length 10

// Test commit/push

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
  int word_batch = 0;
  int word_start = 0;
  int send_to;
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
 
// this stuff should be largely irrelevant for this scheme:
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

 // each worker reads in ALL wiki lines
  if (rank != 0)
  {
    line = (char **) malloc( nlines * sizeof( char * ) );
    for( i = 0; i < nlines; i++ ) 
    {
      line[i] = malloc( 2001 );
    }
    fd = fopen("/homes/dan/625/wiki_dump.txt", "r");
    for (inlines = 0 ; inlines < nlines ; inlines++)
    {
      err = fscanf(fd, "%[^\n]\n", buf );
      if ( buf != NULL)
      {
        strcpy(line[inlines], buf);
      }
    }
    fclose(fd);
  }

// the bulk of task 0 work needs to happen at the same time that it reads in the lines?
  if(rank == 0)
  {
    // first, get it working so that it only reads in 100 words at once,
    // and holds those 100 words in a buffer
    word = (char **) malloc( word_batch *(1+ sizeof( char * )) ); // 100 for 100-word batches
    for( i = 0; i < word_batch; i++ ) 
    {
       word[i] = malloc( word_length );
    }
    fd = fopen("/homes/dan/625/keywords.txt", "r");
    while (nwords_used < nwords)
    {
      // note that this is assuming fscanf holds the place
      // If it does not, then we have to search through the file each time
      // Similar to the worker tasks did for keywords in problem 1.
      for (inwords = 0; inwords < word_batch; inwords++)
      {
         err = fscanf( fd, "%[^\n]\n", word[inwords] );
      }
      MPI_Recv(&send_to, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Status);
      MPI_Send(word, word_length*word_batch, MPI_CHAR, send_to, 0, MPI_COMM_WORLD); 

      // Now need a bit to receive info back from the workers:


      nwords_used = nwords_used + word_batch; // update how many keywords are left
    }

    for (i = 1, i < numtasks; i++)
    {
    // send a message to stop working (break the while(true) on the worker side) 
    }
    fclose( fd );
  } 

  if (rank != 0)
  {
    while (true) 
    {
      MPI_Send(rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // send my ID to task 0
      MPI_Recv(word, word_length*word_batch, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
      // Now do the work, following problem 1 (with hitBuf-like structure)
      // In this case, want a structure that is 100 x 100, for 100 keywords with
      // 100 hits each.
      // May need to think about this more...
    }
  }

  MPI_Finalize();
  return 0;
} // end of main
