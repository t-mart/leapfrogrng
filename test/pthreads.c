#include "test.h"

//3 threads and the main thread
main()  {
  pthread_t f3_thread, f2_thread, f1_thread;
  void *f();
  pthread_create(&f1_thread,NULL,f,NULL);
  pthread_create(&f2_thread,NULL,f,NULL);
  pthread_create(&f3_thread,NULL,f,NULL);
  pthread_join(f1_thread,NULL);
  pthread_join(f2_thread,NULL);
  pthread_join(f3_thread,NULL);
  print_ids();
  r_proc();
  return(0);
}
void *f(){
  print_ids();
  r_proc();
  pthread_exit(0); 
}
