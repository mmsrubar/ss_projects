/*
 * Seminar:     Advanced Operating Systems (POS at BUT)
 * Project:     Ticket algorithm
 * Author:      Michal Srubar, xsruba03@stud.fit.vutbr.cz
 * Date:        Tue Apr 14 09:03:50 CEST 2015
 *
 * Desription:
 * The program creates N threads which uses ticket algorithm for its
 * serialization. Serializator is implemented as three functions:
 *
 * getticket()  - get unique ticket number
 * await()      - enter critical section
 * advance()    - exit critical section
 *
 */
 
#define _POSIX_C_SOURCE 199506L
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1  /* XPG 4.2 - needed for WCOREDUMP() */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#define HALF_SECOND 500000000     /* 500 000 000ns == 0.5s */

#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); return EXIT_FAILURE; } while (0)
/* get number of nanosecond in range of <0, HALF_SECOND> */
#define get_delay(d,s) \
  while ((d = rand_r(&s)) > HALF_SECOND)
/* random sleep in range of <0s, 0.5s> */
#define thread_sleep() {\
  get_delay(delay, seed); \
  delay_time.tv_sec = 0;  \
  delay_time.tv_nsec = delay; \
  nanosleep(&delay_time, NULL); \
}
 
typedef struct {
  unsigned int id;
  pthread_t thr;
} THREAD;

int n;  /* the number of threads */
int m;  /* how many threads can get through the CS */

/* monitor */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond   = PTHREAD_COND_INITIALIZER;
/* shared variables used for ticket algorithm */
int next = 0; /* next ticket number */
int now  = 0; /* current processing ticket number */

int getticket(void)
{
  int ticket;

  pthread_mutex_lock(&mutex);
  ticket = next;
  next++;
  pthread_mutex_unlock(&mutex);

  return ticket;
}

void await(int aenter)
{
  pthread_mutex_lock(&mutex);
  while (aenter != now) {
    while ((pthread_cond_wait(&cond, &mutex)) != 0);
  }
  pthread_mutex_unlock(&mutex);
}

void advance(void)
{
  pthread_mutex_lock(&mutex);
  /* thread with next ticket can ENTER CS */
  now++;
  /* every thread is waiting for its unique ticket */
  pthread_cond_broadcast(&cond);  
  pthread_mutex_unlock(&mutex);
}

void *thread_start(void *par)
{
  int ticket;
  int *id = (int *) par;
  unsigned int seed = (int)getpid() + *id;
  struct timespec delay_time;
  int delay = 0;

  while ((ticket = getticket()) < m) {
    thread_sleep();   
    await(ticket);  /* ENTER CS */
    printf("%d\t(%d)\n", ticket, *id);
    fflush(stdout);
    advance();      /* EXIT CS */
    thread_sleep();
  }

  return NULL;
}

int main(int argc, char *argv[])
{
  int ret, i;
  pthread_attr_t attr;
  THREAD *threads;
  
  if (argc > 2) {
    n = atoi(argv[1]);
    m = atoi(argv[2]);
  } else {
    printf("Usage: %s <n> <m>\n\n", argv[0]);
    printf("n: number of threads in range <0, INT_MAX>\n");
    printf("m: how many times can thread enter the critical section?\n");
    return EXIT_SUCCESS;
  }

  if ((threads = malloc(sizeof(THREAD) * n)) == NULL)
    handle_error_en(errno, "malloc");

  if ((ret = pthread_attr_init(&attr)) != 0)
    handle_error_en(ret, "pthread_attr_init");

  if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) != 0)
    handle_error_en(ret, "pthread_attr_init");

  for (i = 0; i < n; i++) {
    threads[i].id = i;
    if ((ret = pthread_create(&(threads[i].thr), NULL, thread_start, (void *)&(threads[i].id))) != 0)
      handle_error_en(ret, "pthread_create");

  }

  pthread_attr_destroy(&attr);

  for (i = 0; i < n; i++) {
    if ((ret = pthread_join(threads[i].thr, NULL)) != 0)
      handle_error_en(ret, "pthread_create");
  }

  free(threads);
	return 0;
}
