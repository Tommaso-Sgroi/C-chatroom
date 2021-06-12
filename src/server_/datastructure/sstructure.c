#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../datastructures/size.h"

/*struttura sender_msg, tiene le info sul messaggio e l'indirizzo ipv4 di provenienza,
inoltre tiene il sockfd del sender e il numero di byte da scrivere agli altri client*/
typedef struct{
  char message [BUFFER_SIZE_MESSAGE];
  char address [BUFFER_IPV4_SIZE];
  int sockfd, massage_byte;
}sender_msg;
/*struttira client info, contiene l'address del client e il suo clientfd, viene passato in input
al nuovo thread produttore generato per dargli le info base*/
typedef struct{
  char cli_addr [BUFFER_IPV4_SIZE];
  int cli_fd;
}client_info;

/*struttura timestamp in cui vengono posizionate il timestamp che si trova nel messaggio mandato
dal client*/
typedef struct{
  int year, month, day, hours, min, sec;
}timestamp;

/*crea una nuova struttura timestamp*/
timestamp* new_timestamp(char* buffer){
  timestamp* ts = malloc(sizeof(timestamp)); //alloca spazio per la struttura timestamp
  memset(ts, 0, sizeof(timestamp)); //inizializza la struttura a zero

  sscanf(buffer, "%d-%d-%d %d:%d:%d", &ts->year, &ts->month, &ts->day, &ts->hours,  &ts->min, &ts->sec); //estrae dal messaggio il timestamp e lo mette nella struttura
  return ts; //ritorna il puntatore alla struttura timestamp
}
/*crea struttura sender_msg*/
sender_msg* new_sender_msg(char* message, int sockfd, int msg_byte, char* addr){
  
  sender_msg* sender = malloc(sizeof(sender_msg)); //alloca spazio per la struttura
  memset(sender, 0, sizeof(sender_msg)); //inizializza la struttura a zero

  memset(sender->message, 0, BUFFER_SIZE_MESSAGE); //per sicurezza faccio lo stesso per i buffer
  memset(sender->address, 0, BUFFER_IPV4_SIZE);

  strncpy(sender->message, message, strlen(message));//copio il messaggio dallo stack del thread produttore nella struttura
  strncpy(sender->address, addr, strlen(addr)); //copio l'indirizzo del clent dallo stack del thread produttore nella struttura
  sender->sockfd = sockfd; //inserisco il clientfd del client nella struttura
  sender->massage_byte = msg_byte; //inserisco il nuemro di byte letti nella struttura
  return sender; //ritorno il puntatore
}

/*crea struttura client_info per permettere di passare più valori alla creazione di un thread*/
client_info* new_client_info(struct sockaddr_in* addr, int cli_fd){
  client_info* info = malloc(sizeof(client_info)); //alloca lo spazio
  memset(info, 0, sizeof(client_info));//azzera lo spazio
  
  char* address = inet_ntoa(addr->sin_addr);//converte il numero in ASCII per renderlo human readble friendly
  
  memset(info->cli_addr, 0, strlen(address));//azzera per sicurezza i buffer
  strncpy(info->cli_addr, address, strlen(address));
  
  info->cli_fd = cli_fd; //inserisce il client fd
  return info;//ritorna il puntatore
}
