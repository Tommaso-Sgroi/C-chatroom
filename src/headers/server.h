
int setup_server(struct sockaddr_in*, int);
void setup();
  void setup_signal_handler();
  void setup_mutex();
  void setup_cond();
  void setup_logs();
  void setup_arraylist();




void* run_producers(int);
  void* handle_client(void*);
    void append_username(unsigned long);
    void remove_username(unsigned long);
    void append_client_fd(int client_fd);
    void remove_client_fd(int client_fd);
    //void handle_client_accept(struct node*, char*, struct node*, int*, struct linkedlist*);
    int send_users_online(int, char*);

    void append_string_message_to_send(char*, int, int, char*);
    //void append_string_local_log(struct linkedlist*, char*, int);

    //void parse_timestamp(char*, struct tm*);
    unsigned long check_youngest_msg(sender_msg*);

    void send_goodbye(char*, char*, char*);

    //void store_local_log(struct linkedlist*, client_info*, char*);



void* run_consumer(void*);
  void store_message_to_send(char*, int byte_message, char*);
  int check_peer(int, char*);
  void clear_message_to_send();
