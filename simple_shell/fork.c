/*
 * Seminar:     Advanced Operating Systems (POS at BUT)
 * Project:     Simple Shell
 * Author:      Michal Srubar, xsruba03@stud.fit.vutbr.cz
 * Date:        Tue Apr 14 09:03:50 CEST 2015
 *
 * Desription:
 *
 * It supports only MAX_BG_PROCS which can be run in background and only BUF_SIZE
 * long inputs from users.
 */
 
#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 199506L
#define _XOPEN_SOURCE_EXTENDED 1 /* XPG 4.2 - needed for WCOREDUMP() */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <curses.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "cirbuf.h"

#define BUF_SIZE      512 + 1
#define MAX_PROG_ARGS 256
#define PROMPT    "$ "

#define handle_error_en(en, msg, ret) \
  do { errno = en; perror(msg); return ret; } while (0)

/* Circular buffer of processes running in background */
CIRBUFITEM *bg_procs[MAX_BG_PROCS];
/* ========================================================================== */
/* MONITOR */
/* ========================================================================== */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond   = PTHREAD_COND_INITIALIZER;
char buf[BUF_SIZE];   /* shared buffer */
bool quit = false;    /* should the second thread exit? */
int cmd   = 0;        /* non-zero value tells the exec thread that it has some
                         command in buffer it can execute */
bool cont = true;     /* the exec thread tells to the main thread that it can
                         continue reading input from a user */
/* ========================================================================== */

volatile sig_atomic_t sig;
void sigfunc(int signo)
{
  pid_t pid;
  sig = 1;
  pid = wait(NULL);
  cirbuf_set_task_done(bg_procs, pid);
  /*printf("pid exit: %ld\n", (long int)pid);*/
}


/* The command line input is expected in the following format:
 * - the program name is the first
 * - then there can be 0, 1, 2 or N arguments
 * - if char > is specified then all the standard output will go to outpuf_file
 * - if char < is specified then all the input will read from the input_file
 * - if char & is specified then the program will run in background
 *
 *  program arg1 arg2 ... argN [>output_file] [<input_file] [&]
 */
void cmdline_parser(char *buf, char *prog[], char **output, char **input, 
                    bool *bg, char bg_proc_info[])
{
  char *token;
  char *tmp;
  const char *delimiter;
  int i;

  assert(buf != NULL);
  assert(prog != NULL);
  assert(output != NULL);
  assert(input != NULL);
  assert(bg != NULL);

  delimiter = " ";
  *output = NULL;
  *input = NULL;
  *bg = false;
  i = 0;
  memset(prog, '\0', sizeof(char *) * MAX_PROG_ARGS);
  memset(bg_proc_info, '\0', sizeof(char) * BUF_SIZE);

  /* strtok cannot be used on constant strings! */
  if ((token = strtok_r(buf, delimiter, &tmp)) != NULL) {
    /* get the program name first */
    prog[i] = token; i++;
    /* no need to check boundaries because it will never be longer then BUF_SIZE
     * but it would be better to check it!*/
    strcat(bg_proc_info, token);

    while ((token = strtok_r(NULL, delimiter, &tmp)) != NULL) {

      switch (token[0]) {
        case '>':
          if (strlen(token) == 1) {
            /* case: "> output_file" next string should be an output file */
            if ((token = strtok_r(NULL, delimiter, &tmp)) == NULL) {
              break;  /* > without output file */
            }

            *output = token;
            break;
          }
          *output = token+1;
          break;
        case '<':
          if (strlen(token) == 1) {
            /* case: "< output_file" next string should be an output file */
            if ((token = strtok_r(NULL, delimiter, &tmp)) == NULL) {
              break;  /* < without input file */
            }

            *input = token;
            break;
          }
          *input = token+1;
          break;
        case '&':
          *bg = true;
          break;
        default:
          /* a program argument */
          prog[i] = token;
          strcat(bg_proc_info, " ");
          strcat(bg_proc_info, token);
      }

      i++;
    }
  }

  prog[i] = NULL;
}

void *thread_cmd_exec(void *par)
{
  /* An array of string where first is the program name and the others are its
   * arguments. There has to be a NULL after the last program argument. */
  char *prog[MAX_PROG_ARGS];
  /* Information about background process name and its arguments. When a task
   * run in background is done then this information is used to tell what task
   * has compeleted. */
  char bg_proc_info[BUF_SIZE];
  char *output = NULL;
  char *input = NULL;
  bool bg;
  pid_t pid;
  struct sigaction sa;
  sigset_t sb;
  int out_fd, in_fd;

  int task_id = 0;
  CIRBUFITEM *task_p;


  memset(bg_procs, '\0', sizeof(CIRBUFITEM *) * MAX_BG_PROCS);
  while (1) {

    /* ENTRY */
    pthread_mutex_lock(&mutex);

    while (cmd == 0 && quit == false) {
      pthread_cond_wait(&cond, &mutex);
    }

    /* Deny sw interupt from SIGCHLD signal */
    sigemptyset(&sb);
    sigaddset(&sb, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sb, NULL);

    sa.sa_handler = sigfunc;
    sigemptyset(&sa.sa_mask);
    /* otherwise it would interrupt read() in main thread */
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      handle_error_en(errno, "sigaction", NULL);
    }

    sig = 0;  /* now it's save to set this variable */

    if (cmd != 0) {
      cmd--;
      cmdline_parser(buf, prog, &output, &input, &bg, bg_proc_info);
    } else if (quit) {
      /* user typed 'exit' so we can exit this thread now */
      pthread_mutex_unlock(&mutex);
      return NULL;
    }

    pthread_mutex_unlock(&mutex);
    /* EXIT */

    if (prog[0] != NULL && (pid = fork()) == 0) {
      /* child */

      if (output != NULL) {
        if ((out_fd = open(output, O_APPEND | O_CREAT | O_RDWR, 0600)) == -1) {
          handle_error_en(errno, "open", NULL);
        }
        if (dup2(out_fd, fileno(stdout)) == -1) {
          handle_error_en(errno, "dup2", NULL);
        }
      }

      if (input != NULL) {
        if ((in_fd = open(input, O_RDONLY)) == -1) {
          handle_error_en(errno, "open", NULL);
        }
        if (dup2(in_fd, fileno(stdin)) == -1) {
          handle_error_en(errno, "dup2", NULL);
        }
      }

      if (execvp(prog[0], prog) == -1) {
        handle_error_en(errno, "execvp", NULL);
      }

    } else if (pid == -1) {
      perror("fork");
    } 
    
    /* it's safe to work with bg_procs buffer even if the signal handler works
     * with it because we have denied the SIGCHLD signal interrupt */
    if ((bg && (task_id = cirbuf_add(bg_procs, pid, bg_proc_info)) == -1) || bg == false) {
      /* run a process in foreground -> wait until it exit if the the buffer of
       * background processes is full or the program is meant to be in
       * foreground */
      if (task_id == -1) {
        printf("Buffer is full, running the command in foreground.\n");
      }

      while (sig == 0 && sigsuspend(&sa.sa_mask) == -1 && errno == EINTR);
    } 
    else {
      /* run a process in background -> don't wait for SIGCHLD now */
      printf("[%d] %ld\n", task_id, (long int)pid); /* buffer is not full yet */
 
      /* Allow SIGCHLD interrupt */
      sigprocmask(SIG_UNBLOCK, &sb, NULL);
    }

    /* Any done tasks? */
    while ((task_p = cirbuf_get_done_task(bg_procs))) {
      task_id = task_p->task_id;
      printf("[%d]+ Done\t%s\n", task_id, task_p->proc_info); 
      cirbuf_remove_task(bg_procs, bg_procs[task_id]->task_id);
    }

    pthread_mutex_lock(&mutex);
    cont = true;
    pthread_cond_signal(&cond);  
    pthread_mutex_unlock(&mutex);
  }
}

int main(int argc, char *argv[])
{
  int ret;
  int c;

  pthread_t thr;
  if ((ret = pthread_create(&thr, NULL, thread_cmd_exec, NULL)) != 0) {
    handle_error_en(ret, "pthread_create", EXIT_FAILURE);
  }

  while (1) {
   /* wait before the exec thread performs the previous command (only if it
     * wasn't run in background) */
    pthread_mutex_lock(&mutex);
    while (cont == false) {
      pthread_cond_wait(&cond, &mutex);
    }
    cont = false;
    pthread_mutex_unlock(&mutex);

    printf("%s", PROMPT);
    fflush(stdout);

    /* null the entrie buffer first */
    memset(buf, '\0', sizeof(char) * BUF_SIZE);
    ret = read(fileno(stdin), buf, BUF_SIZE);

    if (ret == -1) {
      handle_error_en(errno, "read", EXIT_FAILURE);
    } 
    else if (ret == BUF_SIZE) {
      fprintf(stderr, "Error: The input can't be longer than %dB.\n", BUF_SIZE);
      while ((c = getchar()) != '\n' && c != EOF);   /* discard */ 
    } 
    else if (strlen(buf) == 1 && *buf == '\n') {
      cont = true;
      continue;
    }
    else if (strncmp(buf, "exit", 4) == 0) {
      /* tell the other thread that it's time to quit */
      pthread_mutex_lock(&mutex);
      quit = true;
      pthread_cond_signal(&cond);  
      pthread_mutex_unlock(&mutex);
      break;
    }
    else {
      /* cut off the \n char */
      if (buf[ret-1] == '\n')
        buf[ret-1] = '\0';
      else
        buf[ret] = '\0';

      pthread_mutex_lock(&mutex);
      cmd++;
      pthread_cond_signal(&cond);  
      pthread_mutex_unlock(&mutex);
    }
  }

  pthread_cond_destroy(&cond);
	return EXIT_SUCCESS;
}
