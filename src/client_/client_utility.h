#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../datastructures/size.h"

void print_n_flush(){
  printf(">");
  fflush(stdout);
}

char* str_trim(char* arr, int length) {
  int i;
  for (i = length-1; i >= 0; i--)  // trim \n
    if (arr[i] == '\n'){
      arr[i] = '\0';
      break;
    }
  return arr;
}

char* trim_name(char* name){
  for(int i = strlen(name); i>=0; i--)
    if(name[i] == ':') {
      name[i] = ' ';
      return name;
    }
  return name;
}

char* wrap_message(char* buffer, char* timestamp, char* name, char* message){
  strncat(buffer, timestamp, strlen(timestamp));
  strncat(buffer, name, strlen(name));
  strncat(buffer, message, strlen(message));
  return buffer;
}

int check_valid_name(char* name){
  return strlen(name) <= 16 && strlen(name)>0;
}

void ask_name(char* name){
  printf("%s", "INSERT NAME:\n");
  do
  {
    printf("Name must be higher than 0, less or equals than %d chars\n", 16);
    scanf("%s", name);
    fflush(stdin);
    //printf("%d\n", check_valid_name(name));
  } while (check_valid_name(name)==0);
  strcat(name, ":\n");
}

void setup_log(){
  DIR* dir = opendir("logs");
  if(dir) closedir(dir);
  else if(ENOENT == errno)
  {
      int r = mkdir("logs", 0775);
      if(r < 0) perror("Cannot create /log dir");
      r = chmod("logs", 0775 );
      if(r < 0) perror("Cannot change permission /log dir");
  }
  else
  {
    perror("Impossibile creare dir logs");
    _exit(1);
  }
}

void store_local_log(int fd, char* message){
  if(fd < 0)
  {
    perror("Error while opening local_log: ");
    return;
  }
  //int fd = open("logs/local_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0666);
  int bye_write = write(fd, message, strlen(message));
  if (bye_write < 0)
       perror("ERROR writing in local_log");
}
