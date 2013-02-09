#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

int main()
{
  printf("tgid = %d\n", (int)getpgrp());
  fork();
  sleep(1);
  FILE *fp;
  fp = fopen("/proc/lfrng","r");
  fgetc(fp);
  printf("tid = %d\n", (int)gettid());
  return(0);
}
