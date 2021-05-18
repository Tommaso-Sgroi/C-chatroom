/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>//TO REMOVE?
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include "datastructure/linkedlist.c"
#include "server.h"

#define MAX_CLIENT_QUEUE_REQUEST 16
#define BUFFER_SIZE_MESSAGE 1024

// void error(const char *msg)
// {
//     perror(msg);
//     int exitno = 1;
//     pthread_exit(&exitno);
// }
//PER INTERCETTARE I SIGNAL
// void intHandler(int dummy) {
//     printf("\n%d\n", dummy);
//
// }

struct linkedlist client_fd_linkedlist;

struct {
  struct linkedlist* global_log;
  pthread_cond_t new_message;
} global_log;


int main(int argc, char *argv[]) {
  setup();
  pthread_t tid;

  if(pthread_create(&tid, NULL, &run_consumer, NULL)!=0)
  {
    perror("Cannot create consumer thread");
    exit(1);
  }

  run_producers(/*atoi(argv[1])*/4444);
}

//------------------------RUN------------------------------------------
 int run_producers(int port){
   //declare variables
   int sockfd, client_fd;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   pthread_t tid;

   //setup socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0) perror("ERROR opening socket");

   bzero((char *) &serv_addr, sizeof(serv_addr));

   setup_server(&serv_addr, port);

   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            perror("ERROR on binding");
   //start socket connection
   listen(sockfd, MAX_CLIENT_QUEUE_REQUEST);
   clilen = sizeof(cli_addr);
   while (1)
   {
       client_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
       if (client_fd < 0)
           perror("ERROR on accept");
       if(pthread_create(&tid, NULL, &handle_client, client_fd)!=0)
           perror("Error while making thread");
   }
   close(sockfd);
   return 0; /* we never get here */
 }

int run_consumer(void* null) {

  struct node* last_message = global_log.global_log->first;
  while (1)
  {
    pthread_mutex_lock(&global_log.global_log->mutex);
    puts("Waiting for message\n");
    pthread_cond_wait(&global_log.new_message, &global_log.global_log->mutex);
    /*
      READ MESSAGE
    */
    if(last_message == NULL)//empty linkedlist
      last_message = global_log.global_log->first;
    else last_message = last_message->next;
    while(1)//itero sulla linked list dei log
    {
      //itero sui client_fd
      pthread_mutex_lock(&client_fd_linkedlist.mutex);
      struct node* actual_node = client_fd_linkedlist.first;
      while(actual_node)
      {
        printf("Mando messaggi a %i\n", *(int*)actual_node->value);
        int n = write(*(int*)actual_node->value, (char*)last_message->value, BUFFER_SIZE_MESSAGE); //we can ingore errors on write because
        actual_node = actual_node->next;                      //the thread associated to this fd will close it self the connection
        printf("%i\n", n);
      }
      pthread_mutex_unlock(&client_fd_linkedlist.mutex);
      //fine iterazione sui client_fd
      if(last_message->next)
        last_message = last_message->next; //aggiorno l'ultimo messaggio
      else break;
    }
    printf("Ultimo messaggio: %s\n", last_message->value);
    /*
      READ MESSAGE
    */
    pthread_mutex_unlock(&global_log.global_log->mutex);
  }
  return 0;
}



 /*
   SERVER STUFF
 */

  int setup_server(struct sockaddr_in* serv_addr, int port){
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = INADDR_ANY;
    serv_addr->sin_port = htons(port);
    return 0;
  }

  void setup(){
    make_linkedlist();
    setup_mutex();
    setup_cond();
  }

  void setup_cond() {
    if (pthread_cond_init(&global_log.new_message, NULL) != 0) {
        perror("Error in pthread_cond_init()\n");
        exit(EXIT_FAILURE);
    }
  }

  void setup_mutex(){

    if(pthread_mutex_init(&client_fd_linkedlist.mutex, NULL) != 0 || pthread_mutex_init(&global_log.global_log->mutex, NULL) != 0)
    {
      fprintf(stderr, "Error in pthread_mutex_init()\n");
      exit(EXIT_FAILURE);
    }
  }

  void make_linkedlist(){
    client_fd_linkedlist = *new_linkedlist(NULL);//void linked_list
    global_log.global_log = new_linkedlist(NULL);//void global_log
  }



/*
THREAD CREATE
*/



int handle_client(int client_fd){
  printf("%s: %d\n", "Handling client", client_fd);

  struct linkedlist* local_log = new_linkedlist(NULL);//si può modificare mettendo all'inizio un nodo con l'indirizzo+nome
  struct node* node_client_fd = append_node_client_fd(&client_fd);

  int byte_read;
  char buffer[BUFFER_SIZE_MESSAGE];
  while (1)
  {
    bzero(buffer, BUFFER_SIZE_MESSAGE);
    byte_read = read(client_fd ,buffer, BUFFER_SIZE_MESSAGE);
    if (byte_read <= 0) break; //user disconnected
    printf("Here is the message: %s\n",buffer);
    char* log = append_string_log(buffer, byte_read);//GLOBAL LOG
    append_node(local_log, new_node(log)); //LOCAL LOG, apppend new node that share the same string with global log to decrease the amount of heap used

    // n = write(sock,"I got your message",18);
    // if (n < 0) error("ERROR writing to socket");
  }
  close(client_fd);
  remove_node_client_fd(node_client_fd);
  print_linkedlist(&client_fd_linkedlist);
  //print_linkedlist(&global_log);

  return 0;
}

struct node* append_node_client_fd(int* client_fd){
  struct node* node = new_node((void*)client_fd);

  pthread_mutex_lock(&client_fd_linkedlist.mutex);

  if(append_node(&client_fd_linkedlist, node)!=0)//error occured
    perror("Error while appending node to linkedlist");

  pthread_mutex_unlock(&client_fd_linkedlist.mutex);
  return node;
}

void remove_node_client_fd(struct node* client_fd){
  pthread_mutex_lock(&client_fd_linkedlist.mutex);
  if(remove_node_from_linkedlist(client_fd, &client_fd_linkedlist)!=0)//error occured
    perror("Error while appending node to linkedlist");
  pthread_mutex_unlock(&client_fd_linkedlist.mutex);
}

char* append_string_log(/*struct linkedlist* linkedlist, */char*string, int len){
  pthread_mutex_lock(&global_log.global_log->mutex);

  char* buffer= (char*)malloc(sizeof(char)*len);
  buffer = strncpy(buffer, string, len);
  struct node* node = new_node((void*)buffer);
  if(append_node(global_log.global_log, node))
      perror("Error while appending node to list");

  pthread_cond_signal(&global_log.new_message);
  pthread_mutex_unlock(&global_log.global_log->mutex);
  return buffer;
}
































































































//
