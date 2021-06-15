#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <pthread.h>
#include <time.h> 
#include "datastructures/size.h" 
#include "client_/time/timestamp.c"
#include "client_/struct/client_struct.h"
#include "client_/client_utility.h"
#include "headers/client.h"

/*chiude la connessione e il local log fd*/
void error(const char *msg, int sockfd, int local_log_fd)  
{
    close(sockfd);
    close(local_log_fd);
    perror(msg); 
    exit(0);
}

char null_string[] = " "; //stringa "nulla" contiene uno spazio, serve per utility 
char quit[] = "__quit__\n";  //se il client scrive ciò esce dal server

static char welcome[] = "\n\
██╗    ██╗███████╗██╗      ██████╗ ██████╗ ███╗   ███╗███████╗    ██╗███╗   ██╗████████╗ ██████╗     \n\
██║    ██║██╔════╝██║     ██╔════╝██╔═══██╗████╗ ████║██╔════╝    ██║████╗  ██║╚══██╔══╝██╔═══██╗    \n\
██║ █╗ ██║█████╗  ██║     ██║     ██║   ██║██╔████╔██║█████╗      ██║██╔██╗ ██║   ██║   ██║   ██║    \n\
██║███╗██║██╔══╝  ██║     ██║     ██║   ██║██║╚██╔╝██║██╔══╝      ██║██║╚██╗██║   ██║   ██║   ██║    \n\
╚███╔███╔╝███████╗███████╗╚██████╗╚██████╔╝██║ ╚═╝ ██║███████╗    ██║██║ ╚████║   ██║   ╚██████╔╝    \n\
 ╚══╝╚══╝ ╚══════╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝    ╚═╝╚═╝  ╚═══╝   ╚═╝    ╚═════╝     \n\
                                                                                                     \n\
████████╗██╗  ██╗███████╗                                                                            \n\
╚══██╔══╝██║  ██║██╔════╝                                                                            \n\
   ██║   ███████║█████╗                                                                              \n\
   ██║   ██╔══██║██╔══╝                                                                              \n\
   ██║   ██║  ██║███████╗                                                                            \n\
   ╚═╝   ╚═╝  ╚═╝╚══════╝                                                                            \n\
                                                                                                     \n\
 ██████╗        ██╗  ██╗ █████╗ ████████╗██████╗  ██████╗  ██████╗ ███╗   ███╗                       \n\
██╔════╝        ██║  ██║██╔══██╗╚══██╔══╝██╔══██╗██╔═══██╗██╔═══██╗████╗ ████║                       \n\
██║             ███████║███████║   ██║   ██████╔╝██║   ██║██║   ██║██╔████╔██║                       \n\
██║             ██╔══██║██╔══██║   ██║   ██╔══██╗██║   ██║██║   ██║██║╚██╔╝██║                       \n\
╚██████╗███████╗██║  ██║██║  ██║   ██║   ██║  ██║╚██████╔╝╚██████╔╝██║ ╚═╝ ██║                       \n\
 ╚═════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚═╝     ╚═╝\n";//ASCII art che viene mostrata all'entrata dal server

/*
---------------MAIN-------------------------------------
*/
int main(int argc, char *argv[]){
  //controlli sulla quantità degli argomenti passati
  if(argc > 3)
  {
    fprintf(stderr, "Too many arguments, argument must be the port number\n");
    exit(EXIT_FAILURE);
  }
  else if(argc < 3)
  {
    fprintf(stderr, "Port not found, argument must be the port number\n");
    exit(EXIT_FAILURE);
  }

  setup_log(); //prepara l'ambiente per lo storing dei local log

  int sockfd, portno; //socket file descripto, porta
  struct sockaddr_in serv_addr; //indirizzo del server
  struct hostent server;
  pthread_t tid;

  char name[BUFFER_NAME_SIZE+2]; //bisogna fare scanf con ":\n"
  ask_name(name); //chiede e prende il nome del client
  
  puts(welcome); //stampa la ASCII ART

  portno = atoi(argv[2]); //converte la stringa porta in numero

  sockfd = socket(AF_INET, SOCK_STREAM, 0); //apre una socket
  if(sockfd < 0)
      error("ERROR opening socket", -1, -1);
  
  //server = gethostbyname(/*argv[1]*/"localhost"); //prende l'indirizzo dal nome
  
  if(inet_pton(AF_INET, argv[1], &server) != 1)
  {
    perror("ERROR hostname");
    exit(EXIT_FAILURE);
  }
  
  memset((char *) &serv_addr, 0, sizeof(serv_addr)); //inizializza ls struttura

  serv_addr.sin_family = AF_INET; //inserisce la famiglia
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //inserisce l'indirizzo
  serv_addr.sin_port = htons(portno); //inserisce la porta

  if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) //si connette al server
      error("ERROR connecting", sockfd, -1);
  printf("%s\n", "connection extablished");

  fflush(stdout); //fa un fush di stdin e stuout
  fflush(stdin);

  if(pthread_create(&tid, NULL, &send_message, (void*)new_user_info(sockfd, name/*, "0"*/))!=0) //crea un thread per mandare i messaggi
      perror("Error while making thread");
  listen_message(&sockfd); //si mette in ascolto sui messaggi
  close(sockfd);
  exit(0);
  return 0;
}


void quit_chat(int sockfd, int fd_local_log, char* name){ 
  static char has_left[] = "Has left the chatroom-->"; //stringa da apppendere dietro il nome
  char time [BUFFER_DATE_SIZE]; //buffer del tempo
  char buffer[BUFFER_SIZE_MESSAGE]; //buffer del messaggio

  //inzializza i buffer
  memset(time, 0, BUFFER_DATE_SIZE);
  memset(buffer, 0, BUFFER_SIZE_MESSAGE);

  get_timestamp(time); //prende il timestamp e lo mette in maniera human reaable in time

  strncat(buffer, time, BUFFER_DATE_SIZE); //concatena nel buffer la stringa tempo
  strncat(buffer, has_left, strlen(has_left)); //concatena la stringa has lest prima del nome
  strncat(buffer, name, BUFFER_NAME_SIZE); //concatena la stringa name dopo has left
  strncat(buffer, null_string, strlen(null_string)); //appende la stringa nulla nel posto del messaggio
 
  int bye_write = write(sockfd, buffer, strlen(buffer)); //comunica alla chatroom la sua uscita
  if (bye_write < 0)
       error("ERROR writing to socket", sockfd, fd_local_log);
  store_local_log(fd_local_log, buffer); //salva la sua uscita sul local log

  close(sockfd);//chiude il file descriptor
  close(fd_local_log);//chiude il local log
  exit(EXIT_SUCCESS); //esce con successio
}

/*
-----------------------------LISTEN MESSAGES THREAD----------------------------------
*/

void* listen_message(int* fd){
  int sockfd = *fd; //estrae il socketfd
  char buffer[BUFFER_SIZE_MESSAGE]; //inizializza un buffer per i messaggi in entrata
  while(1)
  {
    memset(buffer, 0, BUFFER_SIZE_MESSAGE); //azzera il buffer
    int n = read(sockfd, buffer, BUFFER_SIZE_MESSAGE); //legge dal server i messaggi e li mette nel buffer
    if (n <= 0)
         error("ERROR reading from socket", sockfd, -1);
    printf("%s\n", str_trim(buffer, n)); //printa il messaggio trimmato
    print_n_flush(); //stampa il carattere > e flusha lo stdout
  }
  return NULL;
}

/*
---------------------------SEND MESSAGES THREAD--------------------------------------
*/


void send_hello(int sockfd, char* name, int fd){
  static char has_joined[]= "JOINED-->"; //stringa da mettere prima del nome quando il client joina

  char buff_out[BUFFER_MESSAGE]; //buffer del messaggio
  char time [BUFFER_DATE_SIZE]; //buffer del tempo
  char new_name[strlen(has_joined) + BUFFER_NAME_SIZE]; //il nome fittizio, stringa joined + nome

  //azzera i contenuti
  memset(buff_out, 0, BUFFER_NAME_SIZE + BUFFER_DATE_SIZE + strlen(has_joined)); 
  memset(time, 0, BUFFER_DATE_SIZE);
  memset(new_name, 0, strlen(has_joined) + BUFFER_NAME_SIZE);

  get_timestamp(time); //prende il timestamp
  strncat(new_name, has_joined, strlen(has_joined)); //concatena la stringa joined nel nuovo nome
  strncat(new_name, name, strlen(name)); //concatena il nome nel nuovo nome

  wrap_message(buff_out, time, trim_name(new_name), null_string); //incapsula il messaggio completo nel buff_out

  int bye_write = write(sockfd, buff_out, strlen(buff_out)); //scrive al server della sua join
  if (bye_write < 0)
       error("ERROR writing to socket", sockfd, fd);
  store_local_log(fd, buff_out); //salva sul local log l'accesso al server
}

/*eseguita dal thread che si occupa di mandare messaggi, 
prende in input una struttura con le info sullo user*/
void* send_message(void* usr_info){
  struct user_info* usr = (struct user_info*)usr_info; //estrae le info
  int fd_local_log = open("logs/local_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0660); //apre il log locale, la dir esiste sempre in questo momento

  int sockfd = usr->fd; //user fd
  char* name = usr->name; //user name

  send_hello(sockfd, name, fd_local_log); //avverte tutti gli altri client dell'arrivo dello user
  puts("Type \"__quit__\" for quit the CHATROOM"); //printa le info per uscire dalla chatroom
  char buffer[BUFFER_SIZE_MESSAGE]; //buffer del messaggio
  
  char timestamp[BUFFER_DATE_SIZE]; //buffer del timestamp
  char message[BUFFER_MESSAGE]; //buffer del messaggio

  while(1)
  {
    //azzera i buffer
    memset(buffer, 0, BUFFER_SIZE_MESSAGE);
    memset(timestamp, 0, BUFFER_DATE_SIZE);
    memset(message, 0, BUFFER_MESSAGE);

    print_n_flush(); //printa '>' e fa il flush dello stdout
    fgets(message, BUFFER_MESSAGE, stdin); //prende il messaggio da tastiera
    
    if(strlen(message) > 1 )//if is not void, ignora automaticamente tutti i messaggi vuoti, per non sovraccaricare il server
    {
      //printf("%s %i\n", message), strcmp(message, quit);
      if(strcmp(message, quit) == 0) //controlla se l'input è di uscita
        quit_chat(sockfd, fd_local_log, usr->name); //esce dalla chatrom
      
      get_timestamp(timestamp); //prende il timestamp un forma human friendly

      char* message_wrapped = wrap_message(buffer, timestamp, name, message); /*fa un wrapping del messaggio
        dove mette il timestamp\n nome: \n messaggio   
        all'interno del buffer del messaggio*/
      
      //char* message_wrapped = wrap_message(buffer, "2020-02-08 15:30:00\n", name, message); 
      /*questo serviva per testare se il server ordinasse bene per timestamp (lo fa)*/

      int bye_write = write(sockfd, message_wrapped, strlen(buffer)); //scrive sul bufffer la lunghezza della stringa
      if (bye_write < 0)
           error("ERROR writing to socket", sockfd, fd_local_log);
      store_local_log(fd_local_log, message_wrapped); //salva in locale il messaggio inviato
    }

  }
}