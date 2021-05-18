#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "size.h"
#include "client.h"

void error(const char *msg, int sockfd)
{
    close(sockfd);
    perror(msg);
    exit(0);
}

struct tm* get_time(){
	time_t t = time(NULL);
	return localtime(&t);
}

static void* listen_message(int* fd){
  int sockfd = *fd;
  char buffer[BUFFER_SIZE_MESSAGE];
  while (1)
  {
    bzero(buffer,BUFFER_SIZE_MESSAGE);
    int n = read(sockfd, buffer, BUFFER_SIZE_MESSAGE-1);
    if (n < 0)
         error("ERROR reading from socket", sockfd);
    printf("%s\n",buffer);
  }
}

static void send_message(void* usr_info){
  struct user_info* usr = (struct user_info*)usr_info;

  int sockfd = usr->fd;
  const char* name = usr->name;

  char buffer[BUFFER_SIZE_MESSAGE + BUFFER_DATE_SIZE + BUFFER_NAME_SIZE];
  while (1)
  {
    //printf("Please enter the message: ");
    bzero(buffer,BUFFER_SIZE_MESSAGE + BUFFER_DATE_SIZE);
    char message[BUFFER_SIZE_MESSAGE];
    fgets(message, BUFFER_SIZE_MESSAGE-1, stdin);

    struct tm tm = *get_time();
    char timestamp[BUFFER_DATE_SIZE];
    snprintf(timestamp, BUFFER_DATE_SIZE, "%d-%d-%d %d:%d:%d%c", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, '\n');

    strcat(buffer, timestamp);
    strcat(buffer, name);
    strcat(buffer, message);

    int bye_write = write(sockfd, buffer, strlen(buffer));
    if (bye_write < 0)
         error("ERROR writing to socket", sockfd);
  }
}



int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_t tid;

    char name[BUFFER_NAME_SIZE]; //bisogna fare scanf con \n
    // if (argc < 3) {
    //    fprintf(stderr,"usage %s hostname port\n", argv[0]);
    //    exit(0);
    // }
    portno = atoi(/*argv[2]*/"4444");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket", -1);
    server = gethostbyname(/*argv[1]*/"localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting", sockfd);
    printf("%s\n", "connection extablished");

    if(pthread_create(&tid, NULL, &send_message, (void*)new_user_info(sockfd, "AAA\n"/*, "0"*/))!=0)
        perror("Error while making thread");
    listen_message(&sockfd);
    close(sockfd);
    exit(0);

    return 0;
}
