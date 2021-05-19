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

void get_time(char* buffer){
  memset(buffer, 0,BUFFER_DATE_SIZE);
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
  snprintf(buffer, BUFFER_DATE_SIZE, "%d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

char* str_trim_nl (char* arr, int length) {
  int i;
  for (i = length-1; i >= 0; i--)  // trim \n
    if (arr[i] == '\n')
    {
      arr[i] = '\0';
      break;
    }
  return arr;
}

void print_n_flush(){
  printf(">>>");
  fflush(stdout);
}

char* wrap_message(char* buffer, char* timestamp, const char* name, char* message){
  strcat(buffer, timestamp);
  strcat(buffer, name);
  strcat(buffer, message);
  return buffer;
}

static void* listen_message(int* fd){
  int sockfd = *fd;
  char buffer[BUFFER_SIZE_MESSAGE];
  while (1)
  {
    memset(buffer, 0, BUFFER_SIZE_MESSAGE);
    int n = read(sockfd, buffer, BUFFER_SIZE_MESSAGE);
    if (n <= 0)
         error("ERROR reading from socket", sockfd);
    printf("%s\n", str_trim_nl(buffer, n));
    print_n_flush();
  }
}

static void send_hello(int sockfd, const char* name){
  char has_joined[]= "JOINED-->";
  char buff_out[strlen(name) + BUFFER_DATE_SIZE + strlen(has_joined)];
  char time [BUFFER_DATE_SIZE];
  char new_name[strlen(has_joined)+BUFFER_NAME_SIZE];

  strcat(new_name, has_joined);
  strcat(new_name, name);

  for(int i = strlen(new_name); i>=0; i--)
    if(new_name[i] == ':') {
        new_name[i] = ' ';
        break;
      }

  get_time(time);
  wrap_message(buff_out, time, new_name, "");

  int bye_write = write(sockfd, buff_out, strlen(buff_out));
  if (bye_write < 0)
       error("ERROR writing to socket", sockfd);
}

static void send_message(void* usr_info){
  struct user_info* usr = (struct user_info*)usr_info;

  int sockfd = usr->fd;
  const char* name = usr->name;

  send_hello(sockfd, name);

  char buffer[BUFFER_SIZE_MESSAGE + BUFFER_DATE_SIZE + BUFFER_NAME_SIZE];
  while (1)
  {

    memset(buffer, 0, BUFFER_DATE_SIZE + BUFFER_NAME_SIZE);

    char message[BUFFER_SIZE_MESSAGE];
    print_n_flush();
    fgets(message, BUFFER_SIZE_MESSAGE-1, stdin);
    if(strlen(message)>1)//if is not void
    {
      char timestamp[BUFFER_DATE_SIZE];
      get_time(timestamp);

      char* message_wrapped = wrap_message(buffer, timestamp, name, message);
      int bye_write = write(sockfd, message_wrapped, strlen(message_wrapped));
      if (bye_write < 0)
           error("ERROR writing to socket", sockfd);
    }

  }
}




int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_t tid;

    char name[BUFFER_NAME_SIZE+2]; //bisogna fare scanf con \n
    printf("%s", "INSERT NAME:\n");
    do
    {
      printf("Name must be higher than 0 and less than %d chars\n", BUFFER_NAME_SIZE-1);
      scanf(" %s", name);
    } while (strlen(name)>BUFFER_NAME_SIZE-1 && strlen(name)>0);
    strcat(name, ":\n");
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
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting", sockfd);
    printf("%s\n", "connection extablished");

    fflush(stdout);
    fflush(stdin);

    if(pthread_create(&tid, NULL, &send_message, (void*)new_user_info(sockfd, name/*, "0"*/))!=0)
        perror("Error while making thread");
    listen_message(&sockfd);
    close(sockfd);
    exit(0);

    return 0;
}
