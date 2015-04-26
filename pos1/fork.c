/*
 * Project:     (POS) Simple demonstration of fork(), wait() and execv()
 * Author:      Michal Srubar, xsruba03@stud.fit.vutbr.cz
 * Date:        Tue Mar 17 09:43:20 CET 2015
 */
 
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1 /* XPG 4.2 - needed for WCOREDUMP() */

void proc_info(const char *label)
{
  printf("%s identification: \n", label); /*grandparent/parent/child */
  printf("pid = %d,ppid = %d,pgrp = %d\n", getpid(), getppid(), getpgrp());
  printf("uid = %d,gid = %d\n", getuid(), getgid());
  printf("euid = %d,egid = %d\n", geteuid(), getegid());
}

void proc_exit(const char *label, pid_t pid, int stat)
{
  printf("%s exit (pid = %d):", label, pid); /* and one line from */

  if (WIFEXITED(stat))
    printf("normal termination (exit code = %d)\n", WEXITSTATUS(stat)); /* or */
  else if (WIFSIGNALED(stat))
#ifdef WCOREDUMP
    printf("signal termination %s(signal = %d)\n", 
        (WCOREDUMP(stat))? "with core dump ":"", WTERMSIG(stat)); /* or */
#else
    printf("signal termination (signal = %d)\n", WTERMSIG(stat)); /* or */
#endif
  else
    printf("unknown type of termination\n");
}

/* ARGSUSED */
int main(int argc, char *argv[])
{
  pid_t parent_pid, child_pid, exit_pid;
  int ret = 0;

  proc_info("grandparrent");

  if ((parent_pid = fork()) == -1) {
    perror("fork()");
    return 1;
  } else if (parent_pid != 0) {
    /* grandparrent */
    if ((exit_pid = wait(&ret)) == -1) {
      perror("wait()");
      return 1;
    }
    proc_exit("parent", exit_pid, ret);
  } else {
    /* parent */
    proc_info("parrent");

    if ((child_pid = fork()) == -1) {
      perror("fork()");
      return 1;
    } else if (child_pid != 0) {
      /* parrent */
      if ((exit_pid = wait(&ret)) == -1) {
        perror("wait()");
        return -1;
      }
      proc_exit("child", exit_pid, ret);
    } else {
      /* child */
      proc_info("child");
      execv(argv[1], argv+1);
      return 0;
    }
  }

  return 0;
}
