#include <stdlib.h>

struct user_info{
  int fd;
  char* name;
  //char* timestamp;
};

struct user_info* new_user_info(int fd, char* name){
  struct user_info* usr =  malloc(sizeof(struct user_info));
  memset(usr, 0, sizeof(struct user_info));
  
  usr->name = name;
  usr->fd = fd;
  return usr;
}
