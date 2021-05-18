#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#define BUFFER_SIZE_MESSAGE 1024

void error(const char *msg, int sockfd)
{
    close(sockfd);
    perror(msg);
    exit(0);
}

static void* listen_message(void* fd){
  int sockfd = *(int*)fd;
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

static void send_message(void* fd){
  int sockfd = *(int*)fd;
  char buffer[BUFFER_SIZE_MESSAGE];
  while (1)
  {
    //printf("Please enter the message: ");
    bzero(buffer,BUFFER_SIZE_MESSAGE);
    fgets(buffer,BUFFER_SIZE_MESSAGE-1,stdin);
    int n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
         error("ERROR writing to socket", sockfd);
  }
}



int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_t tid;

    char buffer[BUFFER_SIZE_MESSAGE];
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

    if(pthread_create(&tid, NULL, &send_message, (void*)&sockfd)!=0)
        perror("Error while making thread");
    listen_message(&sockfd);
    close(sockfd);
    exit(0);

    return 0;
}
