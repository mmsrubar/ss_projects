#ifndef CIRBUF_H
#define CIRBUF_H

#include <sys/types.h>
#include <stdbool.h>

/* how many process can be run in background? */
#define MAX_BG_PROCS  128

typedef struct {
  pid_t pid;
  int task_id;      /* starting from zero to MAX_BG_PROCS  */
  bool running;     /* true = running, false = done */
  char *proc_info;  /* Information about background process name and its
                       arguments. */
} CIRBUFITEM;

/* Find a process with specified pid and set its task as done. */
void cirbuf_set_task_done(CIRBUFITEM *buf[], pid_t pid);

/* Use this in a while loop. Get the task id of first completed task. Return 0
 * if there are no more completed task. */
CIRBUFITEM *cirbuf_get_done_task(CIRBUFITEM *buf[]);

/* Add a new background process to the buffer. Return task_id on success or -1
 * if there is no space left in the buffer or there has been an error with
 * malloc. */
int cirbuf_add(CIRBUFITEM *buf[], pid_t pid, char *proc_info);

/* Remove task with specified task_id. */
void cirbuf_remove_task(CIRBUFITEM *buf[], int id);

void print_debug(CIRBUFITEM *buf[]);
#endif	/* CIRBUF_H */
