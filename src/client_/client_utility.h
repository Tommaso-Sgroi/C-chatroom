#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../datastructures/size.h"

/*printa il carattere '>' e fa un flush dello stdout*/
void print_n_flush(){
  printf(">");
  fflush(stdout);
}

/*toglie l'ultimo carattere '\n' dal messaggio*/
char* str_trim(char* arr, int length) {
  int i; //itera sulla striga fino a trovare '\n' poi fa ul replace con '\0'
  for (i = length-1; i >= 0; i--)  // trim \n
    if (arr[i] == '\n'){
      arr[i] = '\0';
      break;
    }
  return arr; //ritorna la stringa
}
/*toglie l'ultimo carattere ':' dal nome*/
char* trim_name(char* name){
  for(int i = strlen(name); i>=0; i--) //itera sulla stringa
    if(name[i] == ':') {
      name[i] = ' '; //rimpiazza il carattere con uno spazio
      return name;
    }
  return name;
}

/*incapsula le componenti di un messaggio:
  -timestamp
  -nome + ':'
  -messaggio
in un messaggio unico da spedire al server*/
char* wrap_message(char* buffer, char* timestamp, char* name, char* message){
  strncat(buffer, timestamp, BUFFER_DATE_SIZE);//concatena il timestamp
  strncat(buffer, name, BUFFER_NAME_SIZE); //concatena il nome
  strncat(buffer, message, BUFFER_MESSAGE); //concatena il messaggio
  return buffer; //ritorna la stringa
}

int check_valid_name(char* name){
  return strlen(name) <= 16 && strlen(name)>0; //controlla se il nome è ni limiti prestabiliti
}

void ask_name(char* name){
  printf("%s", "INSERT NAME:\n");
  do
  {
    printf("Name must be higher than 0, less or equals than %d chars\n", 16);
    scanf("%s", name); //prende il nome
    fflush(stdin); //svuota il buffer
    //printf("%d\n", check_valid_name(name));
  } while(check_valid_name(name)==0); //controlla se il nome è valido
  strcat(name, ":\n"); /*concatena al nome i caratteri che nel messaggio appariranno come:
  timestamp
  username":\n"
  messaggio*/
}

/*prepara l'ambiente per la creazione dei local log*/
void setup_log(){
  DIR* dir = opendir("logs"); //prova ad prire la directori
  if(dir) closedir(dir); //se esiste la chiude
  else if(ENOENT == errno) //altrimenti se l'errore è "dir not exist"
  {
      int r = mkdir("logs", 0770 ); //...crea la dir...
      if(r < 0) perror("Cannot create /log dir");
  }
  else //l'errore è un altro ed esci perché non è sicuro procedere
  {
    perror("Impossibile creare dir logs");
    _exit(1);
  }
}

/*prende il file descriptor del local log*/
void store_local_log(int fd, char* message){
  if(fd < 0)
  {
    perror("Error while opening local_log: ");
    return;
  }
  int bye_write = write(fd, message, strlen(message)); //scrive messaggio nel local log
  if (bye_write < 0)
       perror("ERROR writing in local_log");
}
