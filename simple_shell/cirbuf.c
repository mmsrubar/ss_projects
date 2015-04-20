/*
 * Seminar:     Advanced Operating Systems (POS at BUT)
 * Project:     Simple Shell
 * Author:      Michal Srubar, xsruba03@stud.fit.vutbr.cz
 * Date:        Tue Apr 14 09:03:50 CEST 2015
 *
 * Desription:
 * This module implements a very simple circular buffer which holds a record of
 * background process. Each record consist of pid of the process, it task id and
 * state which is either running or done.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <assert.h>
#include "cirbuf.h"

/* Remove task with specified task_id. */
static void cirbuf_remove_task(CIRBUFITEM *buf[], int id)
{
  assert(buf != NULL);
  assert(id >= 0 && id < MAX_BG_PROCS);

  free(buf[id]);
  buf[id] = NULL;
}
 
void cirbuf_set_task_done(CIRBUFITEM *buf[], pid_t pid)
{
  int i;

  assert(buf != NULL);
  assert(pid > 0);

  for (i = 0; i < MAX_BG_PROCS; i++) {
    if (buf[i] != NULL && buf[i]->pid == pid) {
      buf[i]->running = false;
      return;
    }
  }
}

int cirbuf_get_done_task(CIRBUFITEM *buf[])
{
  int i;

  assert(buf != NULL);

  for (i = 0; i < MAX_BG_PROCS; i++) {
    if (buf[i] != NULL && buf[i]->running == false) {
      cirbuf_remove_task(buf, buf[i]->task_id);
      return i+1;
    }
  }

  return 0;
}

/* Find and return first non NULL pointer. If the buffer is full then there is
 * no spece left in which case this funcition returns -1.
 */
static int cirbuf_find_index(CIRBUFITEM *buf[])
{
  int i;

  assert(buf != NULL);

  for (i = 0; i < MAX_BG_PROCS; i++) {
    if (buf[i] == NULL) {
      return i;
    }
  }

  return -1;
}

int cirbuf_add(CIRBUFITEM *buf[], pid_t pid)
{
  int index;
  CIRBUFITEM *new;

  assert(buf != NULL);
  assert(pid > 0);

  if ((index = cirbuf_find_index(buf)) == -1) {
    return -1;
  } 
  else {
    if ((new = (CIRBUFITEM *) malloc(sizeof(CIRBUFITEM))) == NULL) {
      perror("malloc");
      return -1;
    }

    new->pid = pid;
    new->task_id = index;
    new->running = true;

    buf[index] = new;
  }

  return new->task_id;
}

void print_debug(CIRBUFITEM *buf[])
{
  int i;

  assert(buf != NULL);

  for (i = 0; i < MAX_BG_PROCS; i++) {
    if (buf[i] != NULL) {
      printf("p[%d]: pid: %ld, state: %c\n", buf[i]->task_id, 
            buf[i]->pid, (buf[i]->running) ? 'R' : 'D');
    }
  }
  putchar('\n');
}

/*
int main(int argc, char *argv[])
{
  CIRBUFITEM *bg_processes[MAX_BG_PROCS] = {[MAX_BG_PROCS-1] = NULL};

  print_debug(bg_processes);

  cirbuf_add(bg_processes, 32);
  cirbuf_add(bg_processes, 54);
  if (cirbuf_add(bg_processes, 54) == -1)
    printf("plno\n");
  print_debug(bg_processes);

  cirbuf_set_task_done(bg_processes, 54);
  printf("task done: %d\n", cirbuf_get_done_task(bg_processes));
  print_debug(bg_processes);

  print_debug(bg_processes);
  return 0;
}
*/
