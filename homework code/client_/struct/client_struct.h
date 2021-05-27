#include <stdlib.h>

struct user_info{
  int fd;
  char* name;
  //char* timestamp;
};

struct user_info* new_user_info(int fd, char* name){
  struct user_info* usr = (struct user_info*) malloc(sizeof(struct user_info));
  usr->name = name;
  usr->fd = fd;
  return usr;
}