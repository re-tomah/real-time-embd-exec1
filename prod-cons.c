/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.	
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>

#define QUEUESIZE 200
#define LOOP 1000
#define PROD 2
#define CONS 2

void *producer (void *args);
void *consumer (void *args);
void *cossCalc(void *args);

typedef struct {
  void * (*work)(void *);
  void * arg;
} workFunction;

typedef struct {
  workFunction **buf;
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

workFunction test;
long add_time[LOOP * PROD];
long pickup_time[LOOP * PROD];
long delays[LOOP * PROD];
struct timeval start, end;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction *in);
bool queueDel (queue *q);

void handle_sigint(int sig)
{ 
 struct timeval tv;
 struct tm* ptm;
 char time_string[40];
 long milliseconds;
 /* Obtain the time of day, and convert it to a tm struct. */
 gettimeofday (&tv, NULL);
 ptm = localtime (&tv.tv_sec);
 /* Format the date and time, down to a single second. */
 strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S.txt", ptm);

  FILE *fptr;
  fptr = fopen(time_string, "w");
   if(fptr == NULL)
   {
      printf("Error!");   
      exit(1);             
   }
  for (int i = 1; i < LOOP*PROD; i++){
    delays[i] = pickup_time[i] - add_time[i];
    fprintf(fptr,"%ld\n",delays[i]);
  }
  fclose(fptr);
  printf("\nFile Closed\n");
}

int main ()
{  
  signal(SIGINT, handle_sigint);

  clock_t start_t;
  start_t = clock();
  queue *fifo;
  pthread_t pro[PROD], con[CONS];
  test.work = cossCalc;
  double k = 1.5;
  test.arg = &k;
  
  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }
  for (int p = 0; p < PROD; p++)
    pthread_create (&pro[p], NULL, producer, fifo);
  for (int c = 0; c < CONS; c++)
    pthread_create (&con[c], NULL, consumer, fifo);
  for (int p = 0; p < PROD; p++){
    pthread_join (pro[p], NULL);
    printf("End\n");}
  for (int c = 0; c < CONS; c++)
    pthread_join (con[c], NULL);
  printf("Done\n");
  queueDelete (fifo);

  struct timeval tv;
  struct tm* ptm;
  char time_string[40];
  long milliseconds;
  /* Obtain the time of day, and convert it to a tm struct. */
  gettimeofday (&tv, NULL);
  ptm = localtime (&tv.tv_sec);
  /* Format the date and time, down to a single second. */
  strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S.txt", ptm);

    FILE *fptr;
    fptr = fopen(time_string, "w");
    if(fptr == NULL)
    {
        printf("Error!");   
        exit(1);             
    }
    for (int i = 1; i < LOOP*PROD; i++){
      delays[i] = pickup_time[i] - add_time[i];
      fprintf(fptr,"%ld\n",delays[i]);
    }
    fclose(fptr);

  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  int i;
  fifo = (queue *)q;

  for (i = 0; i < (LOOP) ; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      // printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    queueAdd (fifo, &test);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
    // printf("produced: %d.\n", i);
  }
  return (NULL);
}

bool br;

void *consumer (void *q)
{
  queue *fifo;
  fifo = (queue *)q;

  while(1) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      // printf ("consumer: queue EMPTY.\n");
      // printf("In function \nthread id = %d\n", pthread_self());
      if (br == 1){ break; }
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    br = queueDel (fifo);
    if (br == 1){ break; }
    // printf("OH\n");
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
  }
  // printf("Hi\n");
  // printf("In function \nthread id = %d\n", pthread_self());

  return (NULL);
}

void *cossCalc(void *ptr)
{
  double trash = cos(*(double *)ptr);
  for (int i = 0; i < 100; i++)
    trash = cos(trash);
  return (NULL);
}


/*
  typedef struct {
  workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  } queue;
*/

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->buf = (workFunction **) malloc (QUEUESIZE * sizeof (workFunction *));
  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

int counter_add = 1;

void queueAdd (queue *q, workFunction *in)
{
  gettimeofday(&start, NULL);
  add_time[counter_add] = (start.tv_sec * 1000000) + start.tv_usec;
  counter_add++;
  q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;
  // printf("Queue Head: %ld\nQueue Tail: %ld\n", q->head, q->tail);

  return;
}

int counter = 0;

bool queueDel (queue *q)
{
  gettimeofday(&end, NULL);
  pickup_time[counter] = (end.tv_sec * 1000000) + end.tv_usec;
  counter++;
  if (counter >= LOOP*PROD){ return 1; }
  printf("%d\n", counter);
  // *out = q->buf[q->head];
  workFunction *test = q->buf[q->head];
  // printf("Deleted head: %ld\n", q->head);
  test->work(test->arg);

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;
  // printf("Queue Head: %ld\nQueue Tail: %ld\n", q->head, q->tail);
  return 0;
}
