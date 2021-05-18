int setup_server(struct sockaddr_in* serv_addr, int port);
int run_producers(int port);
int run_consumer(void* null);
void make_linkedlist();
void setup_mutex();
void setup_cond();
int handle_client(int client_fd);
struct node* append_node_client_fd(int* client_fd);
void remove_node_client_fd(struct node* client_fd);
char* append_string_log(/*struct linkedlist* linkedlist, */char*string, int len);
void setup();


typedef struct{
  char* message;
  int sockfd;
}sender;
