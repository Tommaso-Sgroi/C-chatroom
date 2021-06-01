#include "datastructure/sstructure.h"

int setup_server(struct sockaddr_in*, int);
void setup();
  void setup_signal_handler();
  void setup_mutex();
  void setup_cond();
  void setup_logs();

void make_linkedlist();
  struct node* append_node_client_fd(int*);
  void remove_node_client_fd(struct node*);


void* run_producers(int);
  void* handle_client(void*);
    void get_username(char*, char*);
    int check_username_already_taken(char*);
    void handle_client_name_taken(int);
    void handle_client_accept(struct node*, char*, struct node*, int*, struct linkedlist*);


    void append_string_message_to_send(char*, int, int, char*);
    //void append_string_local_log(struct linkedlist*, char*, int);

    void parse_timestamp(char*, struct tm*);
    struct node* check_youngest_msg(struct node*, struct node*, struct node*);

    void send_goodbye(char*, char*, char*);
    int remove_node_username(struct node*);

    //void store_local_log(struct linkedlist*, client_info*, char*);



void* run_consumer(void*);
  void store_message_to_send(char*, char*, int);
  int check_peer(int, char*);
  void clear_message_to_send();
