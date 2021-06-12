
int setup_server(struct sockaddr_in*, int);
void setup();
  void setup_signal_handler();
  void setup_mutex();
  void setup_cond();
  void setup_logs();
  void setup_arraylist();




void* run_producers(int);
  void* handle_client(void*);
    void append_client_fd(int client_fd);
    void remove_client_fd(int client_fd);
    
    int send_users_online(int, char*);
    
    void append_string_message_to_send(char*, int, int, char*);
    unsigned long check_youngest_msg(sender_msg*);


void* run_consumer(void*);
  void store_message_to_send(char*, int byte_message, char*);
