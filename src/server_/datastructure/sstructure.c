#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../datastructures/size.h"

typedef struct{
  char message [BUFFER_SIZE_MESSAGE];
  char address [BUFFER_IPV4_SIZE];
  int sockfd, massage_byte;
}sender_msg;

typedef struct{
  char cli_addr [BUFFER_IPV4_SIZE];
  int cli_fd;
}client_info;

typedef struct{
  int year, month, day, hours, min, sec;
}timestamp;

timestamp* new_timestamp(char* buffer){
  timestamp* ts = malloc(sizeof(timestamp));
  memset(ts, 0, sizeof(timestamp));

  sscanf(buffer, "%d-%d-%d %d:%d:%d", &ts->year, &ts->month, &ts->day, &ts->hours,  &ts->min, &ts->sec);
  return ts;
}

sender_msg* new_sender_msg(char* message, int sockfd, int msg_byte, char* addr){
  
  sender_msg* sender = malloc(sizeof(sender_msg));
  memset(sender, 0, sizeof(sender_msg));

  memset(sender->message, 0, BUFFER_SIZE_MESSAGE);
  memset(sender->address, 0, BUFFER_IPV4_SIZE);

  strncpy(sender->message, message, strlen(message));
  strncpy(sender->address, addr, strlen(addr));
  sender->sockfd = sockfd;
  sender->massage_byte = msg_byte;
  return sender;
}

client_info* new_client_info(struct sockaddr_in* addr, int cli_fd){
  client_info* info = malloc(sizeof(client_info));
  memset(info, 0, sizeof(client_info));
  
  char* address = inet_ntoa(addr->sin_addr);
  
  memset(info->cli_addr, 0, strlen(address));
  strncpy(info->cli_addr, address, strlen(address));
  
  info->cli_fd = cli_fd;
  return info;
}
