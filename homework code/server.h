int setup_server(struct sockaddr_in* serv_addr, int port);
int run(int port);
int make_linkedlist();
int setup_mutex();
int handle_client(int client_fd);
void append_node_client_fd(int* client_fd);
void append_string_log(struct linkedlist* linkedlist, char string[], int len);
