#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

static inline void print_ids(void)
{
  printf("pgrp = %d, ", (int)getpgrp());
  printf("pgid = %d, ", (int)getpgid());
  printf("ppid = %d, ", (int)getppid());
  printf("pid = %d\n", (int)getpid());
}

static inline void r_proc(void)
{
  FILE *fp;
  fp = fopen("/proc/lfrng","r");
  fgetc(fp);
}

#endif
