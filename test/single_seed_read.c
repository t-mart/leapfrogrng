#include "lfrng.h"

int main(){
lfrng_seed(123, 1);
printf("%d", lfrng_rand());
}
