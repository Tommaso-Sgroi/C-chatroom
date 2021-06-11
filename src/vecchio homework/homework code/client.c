#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include "datastructure/size.h"
#include "client_/time/timestamp.c"
#include "client_/struct/client_struct.h"
#include "client_/client_utility.h"

void error(const char *msg, int sockfd, int local_log_fd)
{
    close(sockfd);
    close(local_log_fd);
    perror(msg);
    exit(0);
}

char null_string[] = " ";
char quit[] = "__quit__\n";

void quit_chat(int sockfd, int fd_local_log, char* name){
  static char has_left[] = "Has left the chatroom-->";
  char time [BUFFER_DATE_SIZE];
  char buffer[BUFFER_SIZE_MESSAGE];

  memset(time, 0, BUFFER_DATE_SIZE);
  memset(buffer, 0, BUFFER_SIZE_MESSAGE);

  get_timestamp(time);

  strncat(buffer, time, BUFFER_DATE_SIZE);
  strncat(buffer, has_left, strlen(has_left));
  strncat(buffer, name, BUFFER_NAME_SIZE);
  strncat(buffer, null_string, strlen(null_string));

  int bye_write = write(sockfd, buffer, strlen(buffer));
  if (bye_write < 0)
       error("ERROR writing to socket", sockfd, fd_local_log);
  store_local_log(fd_local_log, buffer);

  close(sockfd);
  close(fd_local_log);
  exit(EXIT_SUCCESS);
}

/*
-----------------------------LISTEN MESSAGES THREAD----------------------------------
*/

void* listen_message(int* fd){
  int sockfd = *fd;
  char buffer[BUFFER_SIZE_MESSAGE];
  while (1)
  {
    memset(buffer, 0, BUFFER_SIZE_MESSAGE);
    int n = read(sockfd, buffer, BUFFER_SIZE_MESSAGE);
    if (n <= 0)
         error("ERROR reading from socket", sockfd, -1);
    printf("%s\n", str_trim(buffer, n));
    print_n_flush();
  }
  return NULL;
}

/*
---------------------------SEND MESSAGES THREAD--------------------------------------
*/

void send_hello(int sockfd, const char* name, int fd){
  char has_joined[]= "JOINED-->";
  char buff_out[strlen(name) + BUFFER_DATE_SIZE + strlen(has_joined)];
  char time [BUFFER_DATE_SIZE];
  char new_name[strlen(has_joined)+BUFFER_NAME_SIZE];

  get_timestamp(time);
  strncat(new_name, has_joined, strlen(has_joined));
  strncat(new_name, name, BUFFER_NAME_SIZE);

  for(int i = strlen(new_name); i>=0; i--)
    if(new_name[i] == ':') {
      new_name[i] = ' ';
      break;
    }
  wrap_message(buff_out, time, new_name, null_string);

  int bye_write = write(sockfd, buff_out, strlen(buff_out));
  if (bye_write < 0)
       error("ERROR writing to socket", sockfd, fd);
  store_local_log(fd, buff_out);
}



void* send_message(void* usr_info){
  struct user_info* usr = (struct user_info*)usr_info;
  int fd_local_log = open("logs/local_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0666);

  int sockfd = usr->fd;
  const char* name = usr->name;

  //send_hello(sockfd, name, fd_local_log);
  printf("%s\n", "Type \"__quit__\" for quit the CHATROOM");
  char buffer[BUFFER_SIZE_MESSAGE];
  while (1)
  {

    memset(buffer, 0, BUFFER_SIZE_MESSAGE);

    char message[BUFFER_MESSAGE];
    print_n_flush();
    fgets(message, BUFFER_MESSAGE, stdin);
    if(strlen(message)>1)//if is not void
    {
      //printf("%s %i\n", message), strcmp(message, quit);
      if(strcmp(message, quit) == 0)
        quit_chat(sockfd, fd_local_log, usr->name);
      char timestamp[BUFFER_DATE_SIZE];
      get_timestamp(timestamp);

      char* message_wrapped = wrap_message(buffer, timestamp, name, message);
      //char* message_wrapped = wrap_message(buffer, "2020-02-08 15:30:00\n", name, message);

      int bye_write = write(sockfd, message_wrapped, BUFFER_SIZE_MESSAGE);
      if (bye_write < 0)
           error("ERROR writing to socket", sockfd, fd_local_log);
      store_local_log(fd_local_log, message_wrapped);
    }

  }
}



/*
---------------MAIN-------------------------------------
*/
int main(int argc, char *argv[]){
    // if(argc > 2)
    // {
    //   fprintf(stderr, "Too many arguments, argument must be the port number\n");
    //   exit(EXIT_FAILURE);
    // }
    // else if(argc < 2)
    // {
    //   fprintf(stderr, "Port not found, argument must be the port number\n");
    //   exit(EXIT_FAILURE);
    // }

    setup_log();

    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_t tid;

    char name[BUFFER_NAME_SIZE+2]; //bisogna fare scanf con ":\n"
    ask_name(name);
    portno = atoi("6000");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket", -1, -1);
    server = gethostbyname(/*argv[1]*/"localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting", sockfd, -1);
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
