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
#include <signal.h>
#include "server.h"

#define MAX_CLIENT_QUEUE_REQUEST 16

void dostuff(int); /* function prototype */
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
  run(/*atoi(argv[1])*/4444);
}

/******** DOSTUFF() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
   int n;
   char buffer[256];

   while (1) {
     bzero(buffer,256);
     n = read(sock,buffer,255);
     if (n < 0) error("ERROR reading from socket");
     printf("Here is the message: %s\n",buffer);
     n = write(sock,"I got your message",18);
     if (n < 0) error("ERROR writing to socket");
   }
 }

 int setup_server(struct sockaddr_in* serv_addr, int port){
   serv_addr->sin_family = AF_INET;
   serv_addr->sin_addr.s_addr = INADDR_ANY;
   serv_addr->sin_port = htons(port);
   return 0;
 }

 int run(int port){
   int sockfd, newsockfd, portno, pid;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;

   // if (argc < 2) {
   //     fprintf(stderr,"ERROR, no port provided\n");
   //     exit(1);
   // }
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0)
      error("ERROR opening socket");
   bzero((char *) &serv_addr, sizeof(serv_addr));

   setup_server(&serv_addr, port); //port must be taken from argv


   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR on binding");

   listen(sockfd, MAX_CLIENT_QUEUE_REQUEST);
   clilen = sizeof(cli_addr);
   while (1) {
       newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
       if (newsockfd < 0)
           error("ERROR on accept");
       pid = fork(); //TODO USE THREADS
       if (pid < 0)
           error("ERROR on fork");
       if (pid == 0)  {
           close(sockfd);
           dostuff(newsockfd);/*
           USE A CHILD PROCESS THAT POSSES THREADS CHE INTERAGISCONO CON L'UTENTE,
           SI POSSONO TENERE I THREADS IN UNA LINKED LIST CHE CONTIENE LE INFO IL THREAD
           E L'OGGETTO LOG PER GLI UTENTI
           */
           exit(0);
       }
       else close(newsockfd);
   } /* end of while */
   close(sockfd);
   return 0; /* we never get here */
 }
