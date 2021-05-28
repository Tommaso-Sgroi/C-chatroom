#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>//TO REMOVE?
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include "datastructure/linkedlist.c"
#include "server_/server.h"
#include "datastructure/size.h"
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
  //struct node* last_read;
  pthread_cond_t new_message;
} global_log;

void catch_ctrl_c_and_exit(int sig) {
  struct node *clifd = client_fd_linkedlist.first;
  while (clifd)
  {
      close(*(int*)clifd->value); // close all socket include server_sockfd
      clifd = clifd->next;
  }
  exit(EXIT_SUCCESS);
}

/*
-----------------------------MAIN---------------------------------------
*/
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


 /*
   -----------------------------SETUP---------------------------------------
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
    setup_logs();
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

  void setup_logs(){
    DIR* dir = opendir("logs");
    if (dir) closedir(dir);
    else if (ENOENT == errno)
    {
        int r = mkdir("logs", 0775 );
        if(r < 0) perror("Cannot create /log dir");
        r = chmod("logs", 0775 );
        if(r < 0) perror("Cannot change permission /log dir");
    }
    else
    {
      perror("Impossibile creare dir logs");
      exit(1);
    }
  }

  void make_linkedlist(){
    client_fd_linkedlist = *new_linkedlist(NULL);//void linked_list
    global_log.global_log = new_linkedlist(NULL);//void global_log
  }

/*
-----------------------------RUN---------------------------------------
-----------------------------PRODUCERS---------------------------------------
*/
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

   //struct linkedlist* local_log;// = new_linkedlist(NULL);//si può modificare mettendo all'inizio un nodo con l'indirizzo+nome
   struct node* node_client_fd;
   struct node* username_node;

   int byte_read, get_name, error_flag;
   char name[BUFFER_NAME_SIZE+1];
   memset(name, 0, BUFFER_NAME_SIZE);
   char buffer[BUFFER_SIZE_MESSAGE];

   while (1)
   {
     memset(buffer, 0, BUFFER_SIZE_MESSAGE);
     byte_read = read(client_fd ,buffer, BUFFER_SIZE_MESSAGE);
     if (byte_read <= 0) break; //user disconnected
     if(get_name == 0)
     {
       get_username(buffer, name);
       pthread_mutex_lock(&usernames.mutex);
       printf("%s\n", name);
       if(check_username_already_taken(name))
       {
         handle_client_name_taken(client_fd);
         error_flag = 1;
       }
       else //TODO VA SISTEMATO  UN ATTIMO
       {
         username_node = new_node(name);
         append_node(&usernames, username_node); //INTO FUNTION
         node_client_fd = append_node_client_fd(&client_fd);
         //local_log = new_linkedlist(NULL);
       }
       pthread_mutex_unlock(&usernames.mutex);
       get_name++;
       if(error_flag) break;
     }
     append_string_global_log(buffer, byte_read, client_fd, addr);//GLOBAL LOG
   }

   close(client_fd);
   send_goodbye(buffer, name);
   remove_node_client_fd(node_client_fd);
   remove_node_username(username_node);
   free(client_inf);

   return error_flag;
 }

//---------------------------COMUNICATION BETWEN PRODUCERS-CONSUMER-------------------
void append_string_global_log(char*string, int len, int client_fd, char* addr){
  pthread_mutex_lock(&global_log.global_log->mutex);
  char* buffer= (char*)calloc(len, sizeof(char)); //allocate in heap the message (so is not lost at the end of function)
  buffer = strncpy(buffer, string, len);

  sender_msg* msg = new_sender_msg(buffer, client_fd, addr);
  struct node* node = new_node((void*)msg);//new message node
  if(global_log.global_log->lenght) //has next; alias-> there are other messages in queue to be sent
  {                                                       //
    struct node* append_before = NULL;
    struct node* append_node_before = check_youngest_msg(node, global_log.global_log->first, append_before); //node older than new message

    int inserted = insert_first(append_node_before, node); //new node is add first ONLY if is older than anoter msg in queue
    if(global_log.global_log->first == append_node_before && inserted)
      global_log.global_log->first = node;
    else //other msg older than new msg
    {
      printf("APPENDO ALLA FINE\n");
      append_node(global_log.global_log, node);//append node in head of list
    }
  }
  else append_node(global_log.global_log, node); //else there arent msg in queue

  //signal the consumer and release lock
  pthread_cond_signal(&global_log.new_message);
  pthread_mutex_unlock(&global_log.global_log->mutex);
}

struct node* check_youngest_msg(struct node* node, struct node* other, struct node* append_before){
  if(node == NULL || other == NULL || node == other) return NULL;
  sender_msg *sender = (sender_msg*) node->value;
  sender_msg *other_sender = (sender_msg*) other->value;

  struct node* actual_node = other;

	timestamp* sender_ts = new_timestamp(sender->message);
  while(actual_node)
  {
		timestamp* other_ts = new_timestamp(other_sender->message);
    if(sender_ts->year < other_ts->year)
		{
      append_before = actual_node;
      break;
    }
    else if(sender_ts->year == other_ts->year && sender_ts->month < other_ts->month)
    {
      append_before = actual_node;
      break;
    }
    else if(sender_ts->month == other_ts->month && sender_ts->day < other_ts->day)
    {
      append_before = actual_node;
      break;
    }
    else if(sender_ts->day == other_ts->day && sender_ts->hours < other_ts->hours)
    {
      append_before = actual_node;
      break;
    }
    else if(sender_ts->hours == other_ts->hours && sender_ts->min < other_ts->min)
    {
      append_before = actual_node;
      break;
    }
    else if(sender_ts->min == other_ts->min && sender_ts->sec < other_ts->sec) //if sender < reciver
    {
      append_before = actual_node;
      break;
    }
    free(other_ts);
    actual_node = actual_node->next;
  }
  free(sender_ts);
  return append_before;
}


//------------------------CLIENT LOG-IN------------------------------------

/*
Parse username from hello message
*/
 void get_username(char*message, char* name){
   int flag;
   unsigned long int len = strlen(message);
   for(unsigned long int i = 0; i<len; i++)
     if(message[i]=='>')//has joined -->
     {
        do
        {
          i+=1;
          name[flag] = message[i];
          flag++;
        } while(message[i] != '\n' && i < len);
     }
 }

 int check_username_already_taken(char* name){
   struct node* actual_node = usernames.first;
   while(actual_node)
     if(strcmp(name, (char*)actual_node->value) == 0)
      return 1;
     else actual_node = actual_node->next;
   return 0;
 }

 void handle_client_name_taken(int client_fd){
   char name_taken[] = "Name already taken\n";
   write(client_fd, name_taken, strlen(name_taken));
   close(client_fd);
 }

 struct node* append_node_client_fd(int* client_fd){
   struct node* node = new_node((void*)client_fd);

   pthread_mutex_lock(&client_fd_linkedlist.mutex);
   append_node(&client_fd_linkedlist, node);

   pthread_mutex_unlock(&client_fd_linkedlist.mutex);
   return node;
 }

//--------------------USER DISCONNECTED---------------------------------------
 int remove_node_username(struct node* username){
   pthread_mutex_lock(&usernames.mutex);
   int ret = remove_node_from_linkedlist(username, &usernames);
   pthread_mutex_unlock(&usernames.mutex);
   return ret;
 }

 void remove_node_client_fd(struct node* client_fd){
   pthread_mutex_lock(&client_fd_linkedlist.mutex);
   remove_node_from_linkedlist(client_fd, &client_fd_linkedlist);//error occured
   pthread_mutex_unlock(&client_fd_linkedlist.mutex);
 }

 void send_goodbye(char* buffer, char* name/*, struct linkedlist* local_log*/){
   memset(buffer, 0, BUFFER_SIZE_MESSAGE);
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   snprintf(buffer, BUFFER_SIZE_MESSAGE, "%d-%d-%d %d:%d:%d\n%s%s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,"HAS LEFT THE CHATROOM-->", name);

   append_string_global_log(buffer, strlen(buffer), -1, null_addr);//GLOBAL LOG
 }

 /*
  -------------------------------------RUN CONSUMER-------------------------------------
 */

int run_consumer(void* null) {
  int fd = open("logs/global_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0666);
  if(fd < 0)
    perror("Error while opening global log: ");
  while (1)
  {
    pthread_mutex_lock(&global_log.global_log->mutex);
    pthread_cond_wait(&global_log.new_message, &global_log.global_log->mutex);

    pthread_mutex_unlock(&global_log.global_log->mutex);
    sleep(0.3);
    pthread_mutex_lock(&global_log.global_log->mutex);

    while(global_log.global_log->first)//itero sulla linked list dei log
    {
      //itero sui client_fd
      pthread_mutex_lock(&client_fd_linkedlist.mutex);

      struct node* actual_client_fd_node = client_fd_linkedlist.first;
      sender_msg* sender = (sender_msg*)global_log.global_log->first->value;

      while(actual_client_fd_node)
      {
        if(*(int*)actual_client_fd_node->value != sender->sockfd || check_peer(sender->sockfd, sender->addr) != 0) //if false the message has been send by same user so discard
            write(*(int*)actual_client_fd_node->value, sender->message, BUFFER_SIZE_MESSAGE); //we can ingore errors on write because the producer associated will handle it
        actual_client_fd_node = actual_client_fd_node->next;
      }//fine iterazione sui client_fd
      store_global_log(sender->message, sender->addr, fd);
      free(sender->message);
      pthread_mutex_unlock(&client_fd_linkedlist.mutex);
      remove_first_from_linked_list(global_log.global_log);
    }
    pthread_mutex_unlock(&global_log.global_log->mutex);
  }
  return 0;
}

void store_global_log(char* msg, char* cli_address, int fd){
    int byte_w = write(fd, msg, strlen(msg));
    if(byte_w < 0)
      perror("Error while writing logs: ");
    byte_w = write(fd, cli_address, strlen(cli_address));
    if(byte_w < 0)
      perror("Error while writing logs: ");
    byte_w = write(fd, "\n", 1);
    if(byte_w < 0)
      perror("Error while writing logs: ");
}


int check_peer(int client_fd, char* addr_sender){
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  getpeername(client_fd, (struct sockaddr *)&addr, &addr_size);
  return strcmp(inet_ntoa(addr.sin_addr), addr_sender); //cmp ip strings
}
