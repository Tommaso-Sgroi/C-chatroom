#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
