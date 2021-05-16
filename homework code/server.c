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

struct linkedlist client_fd_linkedlist, global_log;


int main(int argc, char *argv[]) {
  run(/*atoi(argv[1])*/4444);
}


 int run(int port){
   //declare variables
   int sockfd, client_fd, portno, pid;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   pthread_t tid;
   make_linkedlist();
   setup_mutex();

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



 /*
   SERVER STUFF
 */

  int setup_server(struct sockaddr_in* serv_addr, int port){
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = INADDR_ANY;
    serv_addr->sin_port = htons(port);
    return 0;
  }

  int setup_mutex(){
    if(pthread_mutex_init(&client_fd_linkedlist.mutex, NULL) != 0)
    {
      fprintf(stderr, "Error in pthread_mutex_init()\n");
      exit(EXIT_FAILURE);
    }
    return 0;
  }

  int make_linkedlist(){
    client_fd_linkedlist = *new_linkedlist(NULL);//void linked_list
    return 0;
  }



/*
THREAD CREATE
*/



int handle_client(int client_fd){
  printf("%s: %d\n", "Handling client", client_fd);

  struct linkedlist* local_log = new_linkedlist(NULL);//si può modificare mettendo all'inizio un nodo con l'indirizzo+nome
  append_node_client_fd(&client_fd);

  int byte_read;
  char buffer[BUFFER_SIZE_MESSAGE];
  while (1)
  {
    bzero(buffer, BUFFER_SIZE_MESSAGE);
    byte_read = read(client_fd ,buffer, BUFFER_SIZE_MESSAGE);
    if (byte_read <= 0) break; //user disconnected

    printf("Here is the message: %s\n",buffer);
    char* log = append_string_log(&global_log, buffer, byte_read);//GLOBAL LOG
    append_node(local_log, new_node(log)); //LOCAL LOG, apppend new node that share the same string with global log to decrease the amount of heap used
  }
  close(client_fd);

  print_linkedlist(local_log);
  print_linkedlist(&global_log);

  return 0;
}

void append_node_client_fd(int* client_fd){
  pthread_mutex_lock(&client_fd_linkedlist.mutex);
  if(append_node(&client_fd_linkedlist, new_node((void*)&client_fd))!=0)//error occured
    perror("Error while appending node to linkedlist");
  pthread_mutex_unlock(&client_fd_linkedlist.mutex);
}

char* append_string_log(struct linkedlist* linkedlist, char*string, int len){
  pthread_mutex_lock(&linkedlist->mutex);

  char* buffer= (char*)malloc(sizeof(char)*len);
  buffer = strncpy(buffer, string, len);
  struct node* node = new_node((void*)buffer);
  if(append_node(linkedlist, node))
      perror("Error while appending node to list");

  pthread_mutex_unlock(&linkedlist->mutex);
  return buffer;
}
































































































//
