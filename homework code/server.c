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
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>//TO REMOVE?
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include "datastructure/linkedlist.c"
#include "server.h"
#include "size.h"

#define MAX_CLIENT_QUEUE_REQUEST 16

// void error(const char *msg)
// {
//     perror(msg);
//     int exitno = 1;
//     pthread_exit(&exitno);
// }

char null_addr []= " ";
struct linkedlist client_fd_linkedlist, usernames;

struct {
  struct linkedlist* global_log;
  struct node* last_read;
  pthread_cond_t new_message;
} global_log;



int main(int argc, char *argv[]) {
  setup();
  pthread_t tid;

  if(pthread_create(&tid, NULL, &run_consumer, NULL)!=0)
  {
    perror("Cannot create consumer thread");
    exit(1);
  }

  run_producers(/*atoi(argv[1])*/4444);
}

//------------------------RUN------------------------------------------
 int run_producers(int port){
   //declare variables
   int sockfd, client_fd;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   pthread_t tid;

   //setup socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0) perror("ERROR opening socket");

   memset((char *) &serv_addr, 0, sizeof(serv_addr));

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
       if(pthread_create(&tid, NULL, &handle_client, (void*)new_client_info(&cli_addr, client_fd))!=0)
           perror("Error while making thread");
   }
   close(sockfd);
   return 0; /* we never get here */
 }

 int handle_client(void* client_inf){
   client_info* client_info_ = (client_info*)client_inf;
   int client_fd = client_info_->cli_fd;
   char* addr = client_info_->cli_addr;

   struct linkedlist* local_log;// = new_linkedlist(NULL);//si può modificare mettendo all'inizio un nodo con l'indirizzo+nome
   struct node* node_client_fd;
   struct node* username_node;

   int byte_read, get_name;
   char name[BUFFER_NAME_SIZE-1];
   char buffer[BUFFER_SIZE_MESSAGE];

   while (1)
   {
     memset(buffer, 0, BUFFER_SIZE_MESSAGE);
     byte_read = read(client_fd ,buffer, BUFFER_SIZE_MESSAGE);
     if (byte_read <= 0) break; //user disconnected
     if(get_name == 0)
     {
       // pthread_mutex_lock(&usernames.mutex);
       get_username(buffer, name);
       int write_b;
       pthread_mutex_lock(&usernames.mutex);
       if(check_username_already_taken(name))
       {
         char* name_taken = "Name already taken\n";
         write_b = write(client_fd, name_taken, strlen(name_taken));
         close(client_fd);
         //remove_node_client_fd(node_client_fd);
         pthread_mutex_unlock(&usernames.mutex);
         return 1;
       }
       else //TODO VA SISTEMATO  UN ATTIMO
       {
         struct node* username = new_node(name);
         append_node(&usernames, username); //INTO FUNTION
         pthread_mutex_unlock(&usernames.mutex);
         node_client_fd = append_node_client_fd(&client_fd);
         local_log = new_linkedlist(NULL);
       }
       printf("%s\n%s\n", buffer, name);
       get_name++;
     }
     append_string_global_log(buffer, byte_read, client_fd, addr);//GLOBAL LOG
     append_string_local_log(local_log, buffer, byte_read); //LOCAL LOG, apppend new node that share the same string with global log to decrease the amount of heap used

     // n = write(sock,"I got your message",18);
     // if (n < 0) error("ERROR writing to socket");
   }
   close(client_fd);
   remove_node_client_fd(node_client_fd);
   send_goodbye(buffer, name, local_log);
   store_local_log(local_log, client_info_, name);
   free(local_log);

   return 0;
 }

 void store_local_log(struct linkedlist* local_log, client_info* info, char* name){
    DIR* dir = opendir("logs");
    if (dir)
    {
      closedir(dir);
      /* Directory exists. */
      char path_prefix [] = "./logs/";
      char path_suffix[] = ".txt";
      int len_path = strlen(path_prefix) + strlen(name)-1 + strlen(path_suffix);
      char path [len_path];
      memset(path, 0, len_path);
      char real_name[strlen(name)-1];
      strncat(real_name, name, strlen(name)-1);
      strncat(path, path_prefix, strlen(path_prefix));
      strncat(path, real_name, strlen(real_name));
      strncat(path, path_suffix, strlen(path_suffix));
      int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0666);
      if(fd < 0)
      {
        perror("Error while opening logs: ");
        return;
      }
      int byte_w = write(fd, info->cli_addr, strlen(info->cli_addr));
      if(byte_w < 0)
      {
        perror("Error while writing logs: ");
        return;
      }
      byte_w = write(fd, "\n", 1);
      if(byte_w < 0)
      {
        perror("Error while writing logs: ");
        return;
      }

      struct node* actual_node = local_log->first;
      while(actual_node)
      {
        char* message = (char*)actual_node->value;
        byte_w = write(fd, message, strlen(message));
        if(byte_w < 0) perror("Error while writing logs: ");
        free(message);
        actual_node = actual_node->next;
      }
      close(fd);

      while (local_log->first)
        remove_node_from_linkedlist(local_log->first, local_log);
      //free(local_log);
    }
    else if (ENOENT == errno)
    {
        /* Directory does not exist. */
        int r = mkdir("logs", 0777);
        if(r < 0) perror("Cannot create /log dir");
        else store_local_log(local_log, info, name); //se tutto va a buon fine allora richiama te stessa così che può ripetere tutti i passaggi correttamente
    }
    else
    {
      /* opendir() failed for some other reason. */
        perror("Cannot open /log dir");
    }
 }

 void send_goodbye(char* buffer, char* name, struct linkedlist* local_log){

   memset(buffer, 0, BUFFER_SIZE_MESSAGE);
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   snprintf(buffer, BUFFER_SIZE_MESSAGE, "%d-%d-%d %d:%d:%d\n%s%s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,"HAS LEFT THE CHATROOM-->", name);
   append_string_global_log(buffer, strlen(buffer), -1, null_addr);//GLOBAL LOG
   append_string_local_log(local_log, buffer, strlen(buffer));
 }


 void get_username(char*message, char* name){
   int flag;
   unsigned long int len = strlen(message);
   for(unsigned long int i = 0; i<len; i++)
   {
     if(message[i]=='>')//has joined -->
     {
        while (message[++i] != '\n')
         name[flag++] = message[i];
       break;
     }
   }
 }

 int check_username_already_taken(char* name){
   struct node* actual_node = usernames.first;
   while(actual_node)
   {
     if(strcmp(name, (char*)actual_node->value) == 0)
      return 1;
     actual_node = actual_node->next;
   }
   return 0;
 }

int run_consumer(void* null) {

  global_log.last_read = global_log.global_log->first;//redundant?
  int fd = open("logs/global_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0666);
  if(fd < 0) {
    perror("Error while opening global log: ");
  }
  while (1)
  {
    pthread_mutex_lock(&global_log.global_log->mutex);
    pthread_cond_wait(&global_log.new_message, &global_log.global_log->mutex);

    if(global_log.last_read == NULL)//empty linkedlist
      global_log.last_read = global_log.global_log->first;
    else global_log.last_read = global_log.last_read->next;

    while(1)//itero sulla linked list dei log
    {
      //itero sui client_fd
      pthread_mutex_lock(&client_fd_linkedlist.mutex);

      struct node* actual_client_fd_node = client_fd_linkedlist.first;
      sender_msg* sender = (sender_msg*)global_log.last_read->value;

      while(actual_client_fd_node)
      {
        if(*(int*)actual_client_fd_node->value != sender->sockfd || check_peer(sender->sockfd, sender->addr) != 0) //if false the message has been send by same user so discard
          {
            write(*(int*)actual_client_fd_node->value, sender->message, BUFFER_SIZE_MESSAGE); //we can ingore errors on write because
          }
        actual_client_fd_node = actual_client_fd_node->next;
      }//fine iterazione sui client_fd
      store_global_log(sender->message, fd);
      pthread_mutex_unlock(&client_fd_linkedlist.mutex);
      if(global_log.last_read->next)
        global_log.last_read = global_log.last_read->next; //aggiorno l'ultimo messaggio
      else break;
    }
    //printf("Ultimo messaggio: %s\n", ((sender_msg*)global_log.last_read->value)->message);

    pthread_mutex_unlock(&global_log.global_log->mutex);
  }
  return 0;
}

void store_global_log(char* msg, int fd){
    int byte_w = write(fd, msg, strlen(msg));
    if(byte_w < 0)
      perror("Error while writing logs: ");
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

  void setup(){
    make_linkedlist();
    setup_mutex();
    setup_cond();
  }

  void setup_cond() {
    if (pthread_cond_init(&global_log.new_message, NULL) != 0) {
        perror("Error in pthread_cond_init()\n");
        exit(EXIT_FAILURE);
    }
  }

  void setup_mutex(){

    if(pthread_mutex_init(&client_fd_linkedlist.mutex, NULL) != 0 || pthread_mutex_init(&global_log.global_log->mutex, NULL) != 0)
    {
      fprintf(stderr, "Error in pthread_mutex_init()\n");
      exit(EXIT_FAILURE);
    }
  }

  void make_linkedlist(){
    client_fd_linkedlist = *new_linkedlist(NULL);//void linked_list
    global_log.global_log = new_linkedlist(NULL);//void global_log
  }



/*
THREAD CREATE
*/





struct node* append_node_client_fd(int* client_fd){
  struct node* node = new_node((void*)client_fd);

  pthread_mutex_lock(&client_fd_linkedlist.mutex);

  if(append_node(&client_fd_linkedlist, node)!=0)//error occured
    perror("Error while appending node to linkedlist");

  pthread_mutex_unlock(&client_fd_linkedlist.mutex);
  return node;
}

void remove_node_client_fd(struct node* client_fd){
  pthread_mutex_lock(&client_fd_linkedlist.mutex);
  if(remove_node_from_linkedlist(client_fd, &client_fd_linkedlist)!=0)//error occured
    perror("Error while appending node to linkedlist");
  pthread_mutex_unlock(&client_fd_linkedlist.mutex);
}

void append_string_global_log(char*string, int len, int client_fd, char* addr){
  pthread_mutex_lock(&global_log.global_log->mutex);

  char* buffer= (char*)calloc(len, sizeof(char)); //allocate in heap the message (so is not lost at the end of function)
  buffer = strncpy(buffer, string, len);

  sender_msg* msg = new_sender_msg(buffer, client_fd, addr);
  struct node* node = new_node((void*)msg);//new message node
  if(global_log.last_read && global_log.last_read->next) //has next; alias-> there are other messages in queue to be sent
  {                                                       //must check also uf at least one message is readed
    struct node* append_before = check_youngest_msg(node, global_log.last_read); //node older than new message
    struct node* tmp = insert_first(append_before, node); //new node is add first ONLY if is older than anoter msg in queue

    if(global_log.global_log->lenght == 1 && tmp && global_log.global_log->first == global_log.last_read) //special case to handle -> if there is 1 msg not send and new msg is older
      global_log.global_log->first = node; //older message is first pos in global_log
    else if(tmp == NULL) //or there arent other msg older than new msg
      append_node(global_log.global_log, node);//append node in head of list
  }
  else append_node(global_log.global_log, node); //else there arent msg in queue

  //signal the consumer and release lock
  pthread_cond_signal(&global_log.new_message);
  pthread_mutex_unlock(&global_log.global_log->mutex);
  //return buffer; //return the heap buffer pointer
}

void append_string_local_log(struct linkedlist* linkedlist, char*string, int len){
  char* buffer= (char*)calloc(len, sizeof(char)); //allocate in heap the message (so is not lost at the end of function)
  buffer = strncpy(buffer, string, len);
  struct node* node = new_node(buffer);//new message node
  append_node(linkedlist, node);
}

// void parse_timestamp(char* buffer, struct tm* parsedTime){
//   int year, month, day, hours, min, sec;
//   // ex: 2009-10-29
//   if(sscanf(buffer, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hours,  &min, &sec) != EOF)
//   {
//     time_t rawTime;
//     time(&rawTime);
//     parsedTime = localtime(&rawTime);
//
//     // tm_year is years since 1900
//     parsedTime->tm_year = year - 1900;
//     // tm_months is months since january
//     parsedTime->tm_mon = month - 1;
//     parsedTime->tm_mday = day;
//     parsedTime->tm_hour = hours;
//     parsedTime->tm_min = min;
//     parsedTime->tm_sec = sec;
//   }
// }

struct node* check_youngest_msg(struct node* node, struct node* other){
  if(node == NULL || other == NULL || node == other) return NULL;
  sender_msg *sender = (sender_msg*) node->value;
  sender_msg *reciver = (sender_msg*) other->value;
  struct node* append_before;
  struct node* actual_node = other;

	timestamp sender_ts = *new_timestamp(sender->message);
  while(actual_node)
  {
		timestamp reciver_ts = *new_timestamp(reciver->message);
    if(sender_ts.year < reciver_ts.year)
			append_before = actual_node;
    else if(sender_ts.year == reciver_ts.year && sender_ts.month < reciver_ts.month)
			append_before = actual_node;
    else if(sender_ts.month == reciver_ts.month && sender_ts.day < reciver_ts.day)
			append_before = actual_node;
    else if(sender_ts.day == reciver_ts.day && sender_ts.hours < reciver_ts.hours)
			append_before = actual_node;
    else if(sender_ts.hours == reciver_ts.hours && sender_ts.min < reciver_ts.min)
			append_before = actual_node;
    else if(sender_ts.min == reciver_ts.min && sender_ts.sec < reciver_ts.sec) //if sender < reciver
			append_before = actual_node;
    actual_node = actual_node->next;
  }
  return append_before;
}



















































































//
