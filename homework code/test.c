#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const *argv[]) {

  int* i = (int*)malloc(sizeof(int));
  printf("%i\n", i);
  *i = 6;
  printf("%i\n", *i);
  free(i);

  printf("%i\n", i);
  printf("%i\n", *i);
  return 0;
}
