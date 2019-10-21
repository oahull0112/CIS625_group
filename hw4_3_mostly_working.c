#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

#define word_length 10
#define batch 100
#define DIETAG 2
#define WORKTAG 1
#define hit_buff_size 100

static void master(void);
static void slave(void);

int main(int argc, char ** argv) {

  // MPI
  int numtasks;
  int rank;
  MPI_Status status;

  MPI_Init( & argc, & argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if ( rank == 0 )
  {
    master();
  }
  else
  {
    slave();
  }

  MPI_Finalize();
}

static void master(void)
{
  int ntasks, rank;
  MPI_Status status;
  int nwords_sent;
  int nwords_total = 200; // change this to take as argument once everything works

  int *hit;
  int **hitBatch;
  char *word;
  char **keywords;
  int i, j, k;

  FILE *fd;
  int inwords;
  int err;
  int work = 1;
  int result; // this will eventually change to be the index buffer

  MPI_Comm_size(MPI_COMM_WORLD, &ntasks);

  // Allocate space to hold a keyword batch:
  fd = fopen("keywords.txt", "r");
  word = (char *)malloc(word_length * batch * sizeof(char *));
  keywords = (char **)malloc(batch*sizeof(char *));
  for (i=0; i<batch; i++)
  {
    keywords[i] = &(word[word_length*i]);
  }

  // Allocate space to receive a hit buffer:
  hit = (int *)malloc(hit_buff_size * batch * sizeof(int)); // cols x rows
  hitBatch = (int **)malloc(batch*sizeof(int *)); // rows
  for (i=0; i<batch; i++)
  {
    hitBatch[i] = &(hit[hit_buff_size*i]);
    for (j = 0; j<hit_buff_size; j++)
    {
      hitBatch[i][j] = -1;
    }
  }

  // queue up and send out an initial word batch to each worker task
  for (rank = 1; rank < ntasks; ++rank)
  {
    // queue up an initial word batch:
    inwords = -1; 
    do {
       err = fscanf( fd, "%[^\n]\n", keywords[++inwords] );
    } while( err != EOF && inwords < batch );

    // Send the initial word batch:
    MPI_Send(&(keywords[0][0]), batch*word_length, MPI_CHAR, rank, WORKTAG, MPI_COMM_WORLD);
    
    nwords_sent = nwords_sent + batch;
  }

  // read in the next word batch:
  inwords = -1; 
  do {
     err = fscanf( fd, "%[^\n]\n", keywords[++inwords] );
  } while( err != EOF && inwords < batch );

  while (err != EOF) // this will need to change to take variable numbers of keywords, not just 50,000
  {
    // receive a result back from a task and send out the next keyword batch:
    MPI_Recv(&(hitBatch[0][0]), hit_buff_size*batch, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Send(&(keywords[0][0]), batch*word_length, MPI_CHAR, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD);

    // batch up the next keyword set
    inwords = -1; 
    do {
       err = fscanf( fd, "%[^\n]\n", keywords[++inwords] );
    } while( err != EOF && inwords < batch );

    nwords_sent = nwords_sent + batch;
//    printf("result: %d \n", result);
  }

  // receive outstanding results, since there is no more work to send out:
  for (rank = 1; rank < ntasks; ++rank)
  {
 //   MPI_Recv(&result, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&(hitBatch[0][0]), hit_buff_size*batch, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  }

  // Tell slaves to die since all work has been received and there is no more to send:
  for (rank = 1; rank < ntasks; ++rank)
  {
    MPI_Send(0, 0, MPI_INT, rank, DIETAG, MPI_COMM_WORLD);
  }

  for (j=0; j<batch; j++)
  {
    printf("\n");
    printf("word %s : ", keywords[j]);
    for (k=0; k<hit_buff_size; k++)
    {
      if( hitBatch[j][k] != -1)
      {
        printf(" %d, ", hitBatch[j][k]);
      }
    }
  }

  return;
}

static void slave(void)
{
  MPI_Status status;
  int i;
  int result; // this will be the index buffer to send
  int inlines;
  int nlines = 500; // just read in 500 wiki lines for now

  int *hit;
  int **hitBatch;
  char *word;
  char **keywords;
  char **line;
  FILE *fd;
  int err;
  int j, k;
  int currentCount = 0; // don't think we actually need this...
  char currentWord[10];

  MPI_Comm_rank(MPI_COMM_WORLD, &result);

  // Allocate space to receive keywords:
  word = (char *)malloc(word_length * batch * sizeof(char *));
  keywords = (char **)malloc(batch*sizeof(char *));
  for (i=0; i<batch; i++)
  {
    keywords[i] = &(word[word_length*i]);
  }

  hit = (int *)malloc(hit_buff_size * batch * sizeof(int)); // cols x rows
  hitBatch = (int **)malloc(batch*sizeof(int *)); // rows
  for (i=0; i<batch; i++)
  {
    hitBatch[i] = &(hit[hit_buff_size*i]);
    for (j = 0; j<hit_buff_size; j++)
    {
      hitBatch[i][j] = -1;
    }
  }

  // read in the wiki lines: (all tasks read in all lines)
  line = (char **) malloc( nlines * sizeof( char * ) );
  for( i = 0; i < nlines; i++ ) {
     line[i] = malloc( 2001 );
  }

  fd = fopen("wiki_dump.txt", "r");
  inlines = -1;
  do {
    err = fscanf( fd, "%[^\n]\n", line[++inlines] );
    } while( err != EOF && inlines < nlines);
  fclose( fd );

  while (1)
  {
    MPI_Recv(&(keywords[0][0]), batch*word_length, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//    printf("tag: %d. Task %d received the words: %s to %s \n", status.MPI_TAG, result, keywords[0], keywords[99]);

    // Check the tag of the received message:
    if (status.MPI_TAG == DIETAG)
    {
      printf("task %d dying now \n", result);
      return;
    }
    // do work here:
    for (j = 0; j < batch; j++)
    {
      for ( k = 0; k < nlines ; k++ )
      {   
        if (strstr(line[k], keywords[j]) != NULL && currentCount < 100 )
        {
          hitBatch[j][currentCount] = k;
          currentCount++;
        }
      }
      currentCount = 0;
    }

//    for (j=0; j<batch; j++)
//    {
//      printf("\n");
//      printf("word %s : ", keywords[j]);
//      for (k=0; k<hit_buff_size; k++)
//      {
//        if( hitBatch[j][k] != -1)
//        {
//          printf(" %d, ", hitBatch[j][k]);
//        }
//      }
//    }
        
   // Send the result back:
   // MPI_Send(&result, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Send(&(hitBatch[0][0]), hit_buff_size*batch , MPI_INT, 0, 0, MPI_COMM_WORLD);
  }

}
