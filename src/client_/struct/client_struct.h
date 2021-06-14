#include <stdlib.h>

/*struttura di tipo user info, contiene il nome e il socketfd su cui scrivere*/
struct user_info{
  int fd;
  char* name;
};


/*crea una struttura user info */
struct user_info* new_user_info(int fd, char* name){
  struct user_info* usr =  malloc(sizeof(struct user_info)); //alloca lo spazio
  memset(usr, 0, sizeof(struct user_info)); //azzera il contenuto
  
  usr->name = name; //inserisce il puntatore al nome
  usr->fd = fd; //inserisce il socket fd su cui scivere
  return usr; //ritorna il puntatore
}
