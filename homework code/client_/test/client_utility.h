#include <stdio.h>
#include <string.h>
#include "../../datastructure/size.h"

void print_n_flush(){
  printf(">");
  fflush(stdout);
}

char* str_trim(char* arr, int length) {
  int i;
  for (i = length-1; i >= 0; i--)  // trim \n
    if (arr[i] == '\n')
    {
      arr[i] = '\0';
      break;
    }
  return arr;
}
// \033[1a lmasdklvsdkmvdsmv \033[1b

char* wrap_message(char* buffer, char* timestamp, const char* name, char* message){
  strncat(buffer, "2021-5-24 12:56:30\n", BUFFER_DATE_SIZE);
  //strncat(buffer, timestamp, BUFFER_DATE_SIZE);
  strncat(buffer, name, BUFFER_NAME_SIZE);
  strncat(buffer, message, BUFFER_SIZE_MESSAGE);
  return buffer;
}

int check_valid_name(char* name){
  for(long unsigned int c = 0; c<strlen(name)-1; c++)
  {
    int ch = name[c];
    if((ch>='0' && ch<='9') || (ch>='A' && ch<='Z') || (ch>='a' && ch<='z') )
      continue;
    else return 0;
  }
  return strlen(name) <= BUFFER_NAME_SIZE-1 && strlen(name)>0;
}

void ask_name(char* name){
  printf("%s", "OLDER INSERT NAME:\n");
  do
  {
    printf("Name must be higher than 0, less than %d chars, and must contains only alphanumeric characters\n", BUFFER_NAME_SIZE-1);
    scanf("%s", name);
    fflush(stdin);
    //printf("%d\n", check_valid_name(name));
  } while (check_valid_name(name)==0);
  strcat(name, ":\n");
}
