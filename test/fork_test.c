#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
  printf("id = %d", (int)getpid());
  return(0);
}
