#include <stdio.h>

int lfrng_rand() {
   int val;
   FILE* LF = fopen("/proc/lfrng", "r");
   fscanf(LF, "%d", val);
   fclose(LF);
   return val;
}

void lfrng_seed(int seed, int num_threads) {
   FILE* LF = fopen("/proc/lfrng", "w");
   fprintf(LF, "%d %d", seed, num_threads);
   fclose(LF);
}
