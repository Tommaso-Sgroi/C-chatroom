#include <stdlib.h>
#include <stdio.h>

typedef struct {
  unsigned long *array;
  size_t used;
  size_t size;

  pthread_mutex_t mutex;
} arraylist;

void initArray(arraylist *a, size_t initialSize) {
  a->array = calloc(initialSize, sizeof(unsigned long));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(arraylist *a, unsigned long element) {
  // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
  // Therefore a->used can go up to a->size 
  if(a->used == a->size) {
    a->size *= 2;
    a->array = realloc(a->array, a->size * sizeof(unsigned long));
  }
  a->array[a->used++] = element;
}

int insertAt(arraylist* a, unsigned long where, unsigned long element){
  if(where < 0 || where >= a->used) return 0;

  if(a->used == a->size) {
    a->size *= 2;
    a->array = realloc(a->array, a->size * sizeof(unsigned long));
  }
  
  for(unsigned long i = a->used; i >= where && i > 0; i--)
    a->array[i] = a->array[i-1];
  
  a->array[where] = element;
  a->used++;

  return 0;
}
void freeArray(arraylist *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}

void trim_list(arraylist* a){
  freeArray(a);
  initArray(a, 10);
}


void removeElement(arraylist* a, unsigned long element){
  short flag = 0;
  for(size_t i = 0; i < a->used; i++){
    
    if(flag) 
      a->array[i-1] = a->array[i]; 

    else if(element == a->array[i]){
      a->array[i] = 0;
      flag++;
    }
  }
  if(flag)
  {
    a->array[a->used-1] = 0;
    a->used--;

    if(a->used == 0 && a->size > 10)
      trim_list(a);
  }
}


// int fakemain(int argc, char const *argv[])
// {
//   arraylist a;
//   int i;

//   initArray(&a, 5);  // initially 5 elements
//   for (i = 0; i < 10; i++)
//     insertArray(&a, i);  // automatically resizes as necessary
//   printf("usata: %ld\n", a.used);  // print number of elements

//   // removeElement(&a, 1);  
//   // removeElement(&a, 5);
//   // removeElement(&a, 9);

//   insertAt(&a, 0, 100);

//   printf("usata dopo aggiunta: %ld\n", a.used);  // print number of elements

//   for (i = 0; i < a.used; i++)
//     printf("%d\n", a.array[i]);
//   printf("size totale: %lu\n", a.size);
//   return 0;
// }

// int main(int argc, char const *argv[])
// {
//   fakemain(argc, argv);
//   return 0;
// }

