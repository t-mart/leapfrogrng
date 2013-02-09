#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

static inline void print_ids(void)
{
  //printf("pgrp = %d, ", (int)getpgrp());
  printf("pid = %d, ", (int)getpid());
  printf("tid = %d\n", syscall(SYS_gettid));
}

static inline void r_proc(void)
{
  FILE *fp;
  fp = fopen("/proc/lfrng","r");
  fgetc(fp);
}

#endif
