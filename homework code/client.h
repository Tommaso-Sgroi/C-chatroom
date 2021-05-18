#include <time.h>
#include <stdlib.h>



unsigned long get_timestamp(){
  return (unsigned long)time(NULL);
}

struct user_info{
  int fd;
  char* name;
  char* timestamp;
};

struct user_info* new_user_info(int fd, char* name/*, char* timestamp*/){
  struct user_info* usr = (struct user_info*) malloc(sizeof(struct user_info));
  usr->name = name;
  //usr->timestamp = timestamp;
  usr->fd = fd;
  return usr;
}
