#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

#define word_length 10
#define batch 100
#define DIETAG 2
#define WORKTAG 1
#define BATCH_TAG 4
#define OFFSET_TAG 3
#define hit_buff_size 100

static void master(void);
static void slave(void);

double myclock();

int numberWords, numberLines;

// program set-up adapted from https://web.archive.org/web/20150209220259/http://www.lam-mpi.org/tutorials/one-step/ezstart.php
int main(int argc, char ** argv) {
  double tstart, ttotal;

  // MPI
  int numtasks;
  int rank;
  MPI_Status status;

  tstart = myclock(); // set the zero time
  tstart = myclock(); // start the clock

  if (argc == 3)
  {
    numberWords = atoi(argv[1]);
    numberLines = atoi(argv[2]);
  }

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

  ttotal = myclock() - tstart;
  if (rank == 0)
  {
    printf("\n");
    printf("DATA, 3, %lf\n", ttotal);
  }

  MPI_Finalize();
}

static void master(void)
{
  int ntasks, rank;
  MPI_Status status;

  int *hit;
  int **hitBatch;
  char *word;
  char **keywords;
  int i, j, k;
  char buf[10];

  FILE *fd;
  int inwords;
  int err;
  int work = 1;
  int result; // this will eventually change to be the index buffer
  int offset = 0;
  int offset_rec;
  int ikeys = 0;
  char **full_keywords;
  int icWords = 0; // "index of current words"

  MPI_Comm_size(MPI_COMM_WORLD, &ntasks);


  // Allocate space for full keyword list:
  full_keywords = (char **) malloc(numberWords *(1+sizeof(char *)));
  for (i=0; i<numberWords; i++)
  {
    full_keywords[i] = malloc(word_length);
  }

  // Allocate space to hold a keyword batch:
  fd = fopen("/homes/dan/625/keywords.txt", "r");
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
  inwords = 0; 
    while ( err != EOF && inwords < batch && ikeys < numberWords)
    {
       err = fscanf( fd, "%[^\n]\n", keywords[inwords] );
       strcpy(full_keywords[ikeys], keywords[inwords]);
       ikeys++;
       inwords++;
    }

    // Send the initial word batch:
    MPI_Send(&(keywords[0][0]), batch*word_length, MPI_CHAR, rank, WORKTAG, MPI_COMM_WORLD);
    MPI_Send(&offset, 1, MPI_INT, rank, OFFSET_TAG, MPI_COMM_WORLD);
    offset++;
    icWords = icWords + batch;
    
  }

  // read in the next word batch:
  inwords = 0; 
  while ( err != EOF && inwords < batch && ikeys < numberWords)
  {
     err = fscanf( fd, "%[^\n]\n", keywords[inwords] );
     strcpy(full_keywords[ikeys], keywords[inwords]);
     ikeys++;
     inwords++;
  }

  while ( icWords < numberWords) // icWords = "index of current Words"
  {
    // receive a result back from a task and send out the next keyword batch:
    MPI_Recv(&(hitBatch[0][0]), hit_buff_size*batch, MPI_INT, MPI_ANY_SOURCE, BATCH_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&offset_rec, 1, MPI_INT, MPI_ANY_SOURCE, OFFSET_TAG, MPI_COMM_WORLD, &status);
    MPI_Send(&(keywords[0][0]), batch*word_length, MPI_CHAR, status.MPI_SOURCE, WORKTAG, MPI_COMM_WORLD);
    MPI_Send(&offset, 1, MPI_INT, status.MPI_SOURCE, OFFSET_TAG, MPI_COMM_WORLD);
    offset++;
    icWords = icWords + batch;


    // batch up the next keyword set
    inwords = 0; 
    while ( err != EOF && inwords < batch && ikeys < numberWords)
    {
       err = fscanf( fd, "%[^\n]\n", keywords[inwords] );
       strcpy(full_keywords[ikeys], keywords[inwords]);
       ikeys++;
       inwords++;
    }

    for (j=0; j<batch; j++)
    {
      printf("\n");
      printf("%s", full_keywords[j+offset_rec*batch]);
      for (k=0; k<hit_buff_size; k++)
      {
        if (hitBatch[j][k] != -1)
        {
          printf(", %d", hitBatch[j][k]);
        }
      }
    }
  }

  // receive outstanding results, since there is no more work to send out:
  for (rank = 1; rank < ntasks; ++rank)
  {
    MPI_Recv(&(hitBatch[0][0]), hit_buff_size*batch, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&offset_rec, 1, MPI_INT, MPI_ANY_SOURCE, OFFSET_TAG, MPI_COMM_WORLD, &status);
    for (j=0; j<batch; j++)
    {
      printf("\n");
      printf("%s", full_keywords[j+offset_rec*batch]);
      for (k=0; k<hit_buff_size; k++)
      {
        if (hitBatch[j][k] != -1)
        {
          printf(", %d", hitBatch[j][k]);
        }
      }
    }
  }

  // Tell slaves to die since all work has been received and there is no more to send:
  for (rank = 1; rank < ntasks; ++rank)
  {
    MPI_Send(0, 0, MPI_INT, rank, DIETAG, MPI_COMM_WORLD);
  }

  return;
}

static void slave(void)
{
  MPI_Status status;
  int i;
  int result; // this will be the index buffer to send
  int inlines;
  int *hit;
  int **hitBatch;
  char *word;
  char **keywords;
  char **line;
  FILE *fd;
  int err;
  int j, k;
  int currentCount = 0; // don't think we actually need this...
  int offset;
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

  // allocate memory and read in the wiki lines: (all worker tasks read in all lines)
  line = (char **) malloc( numberLines * sizeof( char * ) );
  for( i = 0; i < numberLines; i++ ) {
     line[i] = malloc( 2001 );
  }
  fd = fopen("/homes/dan/625/wiki_dump.txt", "r");
  inlines = -1;
  do {
    err = fscanf( fd, "%[^\n]\n", line[++inlines] );
    } while( err != EOF && inlines < numberLines);
  fclose( fd );

  // While there is work available, receive work from master task
  while (1)
  {
    MPI_Recv(&(keywords[0][0]), batch*word_length, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    // Check the tag of the received message. DIETAG says there is no more work, so exit.
    if (status.MPI_TAG == DIETAG)
    {
      return;
    }
    MPI_Recv(&offset, 1, MPI_INT, 0, OFFSET_TAG, MPI_COMM_WORLD, &status);
    // do work:
    for (j = 0; j < batch; j++)
    {
      for ( k = 0; k < numberLines ; k++ )
      {   
        if (strstr(line[k], keywords[j]) != NULL && currentCount < 100 )
        {
          hitBatch[j][currentCount] = k;
          currentCount++;
        }
      }
      currentCount = 0;
    }

   // Send the result back:
    MPI_Send(&(hitBatch[0][0]), hit_buff_size*batch , MPI_INT, 0, BATCH_TAG, MPI_COMM_WORLD);
    MPI_Send(&offset, 1, MPI_INT, 0, OFFSET_TAG, MPI_COMM_WORLD);
  }

}

double myclock() {
 static time_t t_start = 0;  // Save and subtract off each time
 struct timespec ts;        

 clock_gettime(CLOCK_REALTIME, &ts);
 if( t_start == 0 ) t_start = ts.tv_sec;        

 return (double) (ts.tv_sec - t_start) + ts.tv_nsec * 1.0e-9;
}

