#include <stdio.h>
#include <pthread.h> 
#include <sys/types.h>
#include <unistd.h>

main()  {
  pthread_t f2_thread, f1_thread;
  void *f();
  pthread_create(&f1_thread,NULL,f);
  pthread_create(&f2_thread,NULL,f);
  pthread_join(f1_thread,NULL);
  pthread_join(f2_thread,NULL);
}
void *f(void){
  FILE *fp;
  fp = fopen("/proc/lfrng","r");
  fgetc(fp);
  printf("tgid = %d\n", (int)getpgrp());
  printf("tid = %d\n", (int)getpid());
  pthread_exit(0); 
}
