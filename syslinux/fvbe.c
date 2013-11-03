#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int main(void)
{
  printf("Hello World!\n");
  sleep(10);
  return EXIT_SUCCESS;
}
