#include <stdio.h>

int lfrng_rand() {
   int val = -1;
   FILE *LF = fopen("/proc/lfrng", "r");
   if (LF != NULL){
     fscanf(LF, "%d", val);
     fclose(LF);
   }
   return val;
}

void lfrng_seed(long long unsigned int seed, int num_threads) {
   FILE *LF = fopen("/proc/lfrng", "w");
   if (LF != NULL){
     fprintf(LF, "%llu %d\n", seed, num_threads);
     fclose(LF);
   }
}
