#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "datastructure/size.h"
#include "client_/time/timestamp.h"
#include "client_/struct/client_struct.h"
#include "client_/client_utility.h"

void error(const char *msg, int sockfd, int local_log_fd)
{
    close(sockfd);
    close(local_log_fd);
    perror(msg);
    exit(0);
}


/*
-----------------------------LISTEN MESSAGES THREAD----------------------------------
*/

static void* listen_message(int* fd){
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
}

/*
---------------------------SEND MESSAGES THREAD--------------------------------------
*/

static void send_hello(int sockfd, const char* name, int fd){
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
  wrap_message(buff_out, time, new_name, "");

  int bye_write = write(sockfd, buff_out, strlen(buff_out));
  if (bye_write < 0)
       error("ERROR writing to socket", sockfd, fd);
  store_local_log(fd, buff_out);
}



static void send_message(void* usr_info){
  struct user_info* usr = (struct user_info*)usr_info;
  int fd_local_log = open("logs/local_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0666);

  int sockfd = usr->fd;
  const char* name = usr->name;

  send_hello(sockfd, name, fd_local_log);

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
      get_timestamp(timestamp);

      char* message_wrapped = wrap_message(buffer, timestamp, name, message);
      //char* message_wrapped = wrap_message(buffer, "2020-02-08 15:30:00\n", name, message);

      int bye_write = write(sockfd, message_wrapped, strlen(message_wrapped));
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
    setup_log();

    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    pthread_t tid;

    char name[BUFFER_NAME_SIZE+2]; //bisogna fare scanf con ":\n"
    ask_name(name);
    // if (argc < 3) {
    //    fprintf(stderr,"usage %s hostname port\n", argv[0]);
    //    exit(0);
    // }
    portno = atoi(/*argv[2]*/"4444");

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
