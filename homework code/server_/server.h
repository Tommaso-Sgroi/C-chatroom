#include "datastructure/sstructure.h"

int setup_server(struct sockaddr_in*, int);
void setup();
  void setup_mutex();
  void setup_cond();
  void setup_logs();

void make_linkedlist();
  struct node* append_node_client_fd(int*);
  void remove_node_client_fd(struct node*);


int run_producers(int);
  int handle_client(void*);
    void get_username(char*, char*);
    int check_username_already_taken(char*);
    void handle_client_name_taken(int);
    void handle_client_accept(struct node*, char*, struct node*, int*, struct linkedlist*);


    void append_string_global_log(char*, int, int, char*);
    //void append_string_local_log(struct linkedlist*, char*, int);

    void parse_timestamp(char*, struct tm*);
    struct node* check_youngest_msg(struct node*, struct node*, struct node*);

    void send_goodbye(char*, char*/*, struct linkedlist**/);
    int remove_node_username(struct node*);

    //void store_local_log(struct linkedlist*, client_info*, char*);



int run_consumer(void*);
  void store_global_log(char*, char*, int);
  int check_peer(int, char*);
  void clear_global_log();
