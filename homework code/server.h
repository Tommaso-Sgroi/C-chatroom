#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

int setup_server(struct sockaddr_in* serv_addr, int port);
int run_producers(int port);
int run_consumer(void* client_info);
void make_linkedlist();
void setup_mutex();
void setup_cond();
int handle_client(void* client_info);
struct node* append_node_client_fd(int* client_fd);
void remove_node_client_fd(struct node* client_fd);
char* append_string_log(/*struct linkedlist* linkedlist, */char*string, int len, int client_fd, char* addr);
void setup();
void parse_timestamp(char* buffer, struct tm* parsedTime);
struct node* check_youngest_msg(struct node* node, struct node* other);
void get_username(char*message, char* name);
void send_goodbye(char* buffer, char* name, struct linkedlist* local_log);


typedef struct{
  char* message;
  int sockfd;
  char* addr;
}sender_msg;

typedef struct{
  char* cli_addr;
  int cli_fd;
}client_info;

typedef struct{
  int year, month, day, hours, min, sec;
}timestamp;

timestamp* new_timestamp(char* buffer){
  timestamp* ts = (timestamp*) malloc(sizeof(timestamp));
  sscanf(buffer, "%d-%d-%d %d:%d:%d", &ts->year, &ts->month, &ts->day, &ts->hours,  &ts->min, &ts->sec);
  return ts;
}

sender_msg* new_sender_msg(char* message, int sockfd, char* addr){
  char* address = (char*) calloc(strlen(addr), sizeof(char));
  strncpy(address, addr, strlen(addr));
  sender_msg* sender =(sender_msg*) malloc(sizeof(sender_msg));
  sender->message = message;
  sender->sockfd = sockfd;
  sender-> addr = address;
  return sender;
}

client_info* new_client_info(struct sockaddr_in * addr, int cli_fd){
  client_info* info = (client_info*)malloc(sizeof(client_info));
  info->cli_addr = inet_ntoa(addr->sin_addr);
  info->cli_fd = cli_fd;
  return info;
}

int check_peer(int client_fd, char* addr_sender){
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  getpeername(client_fd, (struct sockaddr *)&addr, &addr_size);
  return strcmp(inet_ntoa(addr.sin_addr), addr_sender); //cmp ip strings
}
