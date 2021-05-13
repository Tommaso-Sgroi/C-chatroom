#include <stdlib.h>
#include <stdio.h>
#define MAX_STRING_LEN 1024
#define START_ARRAY_LEN 10



struct arraylist{
  void* array;
  int real_lenght;
  int used_leght;
};

struct arraylist* new_array_list(){
  struct arraylist* arraylist = (struct arraylist*)malloc(sizeof(struct arraylist));
  arraylist->real_lenght = START_ARRAY_LEN;
  return arraylist;
}

struct arraylist* new_array_list_from_array_list(struct arraylist* new_array_list, struct arraylist* old_array_list){
  return NULL;
}

int append_string(char* string, struct arraylist* arraylist){
  if(arraylist->real_lenght == arraylist->used_leght)
  {
      int new_size = sizeof(arraylist->array)*2;
      arraylist->array = (void*) realloc(arraylist->array, new_size);
  }
  //char* array [arraylist->real_lenght][MAX_STRING_LEN] = arraylist->array;

  return 0;
}

int main(int argc, char const *argv[]) {



  return 0;
}
