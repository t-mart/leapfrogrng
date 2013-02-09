#include <omp.h>
#include "test.h"

#define THREADS_TO_USE 4

int main (int argc, char *argv[]) 
{
int nthreads, tid;

omp_set_dynamic(0);     // Explicitly disable dynamic teams
omp_set_num_threads(THREADS_TO_USE); // Use 4 threads for all consecutive parallel regions
/*[> Fork a team of threads giving them their own copies of variables <]*/
#pragma omp parallel private(nthreads, tid)
/*[>#pragma omp threadprivate(nthreads, tid)<]*/
  {

  /*[> Obtain thread number <]*/
  tid = omp_get_thread_num();
  print_ids();
  r_proc();
  /*[>printf("openmptid = %d ", tid);<]*/

  /*[> Only master thread does this <]*/
  /*if (tid == 0) */
    /*{*/
    /*nthreads = omp_get_num_threads();*/
    /*printf("Number of threads = %d ", nthreads);*/
    /*printf(" ^That was master thread\n");*/
    /*}*/

  }
  return(0);

}
