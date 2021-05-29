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

struct linkedlist client_fd_linkedlist, usernames; //linkedlist per contenere client_fd e gli usernames in modo dinamico

struct {
  struct linkedlist* message_to_send; //linkedlist dei messaggi che verranno mandati dal consumer
  //struct node* last_read;
  pthread_cond_t new_message; //segnala al thread consumatore che è arrivato un nuovo messaggio
} message_to_send_struct; //serve solo una unica istanza di questa struttura in cui vengono inseriti i messaggi in arrivo ordinati secondo timestamp

// void catch_ctrl_c_and_exit(int sig) { //serve per uscire in modo
//   printf("CATCH\n");
//   struct node *clifd = client_fd_linkedlist.first;
//   while (clifd)
//   {
//       close(*(int*)clifd->value); // close all socket include server_sockfd
//       clifd = clifd->next;
//   }
//   exit(EXIT_SUCCESS);
// }

/*
-----------------------------MAIN---------------------------------------
*/
int main(int argc, char *argv[]) {
  setup(); //crea le liste linkate, crea le mutex, crea le conditions e crea l'ambiente per i log
  pthread_t tid; //thread id del consumatore

  if(pthread_create(&tid, NULL, &run_consumer, NULL)!=0) //crea il thread consumatore
  {
    perror("Cannot create consumer thread");
    exit(1);
  }

  run_producers(/*atoi(argv[1])*/4444); //continua l'esecuzione come produttore
}


 /*
   -----------------------------SETUP---------------------------------------
 */

  int setup_server(struct sockaddr_in* serv_addr, int port){
    serv_addr->sin_family = AF_INET; //permette di fare binding sulla porta
    serv_addr->sin_addr.s_addr = INADDR_ANY; //permette connessioni da tutti gli indirizzi
    serv_addr->sin_port = htons(port); //imposta la porta
    return 0;
  }

  void setup(){
    make_linkedlist();
    setup_mutex();
    setup_cond();
    setup_logs();
  }

  void setup_cond() {
    if (pthread_cond_init(&message_to_send_struct.new_message, NULL) != 0)//inizializza la condition di arrivo messaggio
    {//se da errore non è sicuro continuare
        perror("Error in pthread_cond_init()\n");
        exit(EXIT_FAILURE);
    }
  }

  void setup_mutex(){

    if(pthread_mutex_init(&client_fd_linkedlist.mutex, NULL) != 0 || pthread_mutex_init(&message_to_send_struct.message_to_send->mutex, NULL) != 0) //inizializza le mutex per le liste linkate del client_fd e i messaggi da mandare
    { //se uno dei 2 da errore allora non è sicuro avviare il server
      fprintf(stderr, "Error in pthread_mutex_init()\n");
      exit(EXIT_FAILURE);
    }
  }

  void setup_logs(){
    DIR* dir = opendir("logs");/*fa un check per vedere se esiste la dir dei log provando ad aprirla*/
    if(dir) closedir(dir); //se esiste allora chiudila perché non serve tenerla aperta
    else if(ENOENT == errno)//altrimenti sc'è l'errore di apertura...
    {
        int r = mkdir("logs", 0775 ); //...crea la dir...
        if(r < 0) perror("Cannot create /log dir");
        r = chmod("logs", 0775 );//...reimposta i diritti
        if(r < 0) perror("Cannot change permission /log dir");
    }
    else
    {//altrimenti l'errore è dato da qualche altro motivo e non è sicuro andare avanti (si esce)
      perror("Impossibile creare dir logs");
      exit(1);
    }
  }

  void make_linkedlist(){
    client_fd_linkedlist = *new_linkedlist(NULL);//void linked_list
    message_to_send_struct.message_to_send = new_linkedlist(NULL);//void message_to_send
  }

/*
-----------------------------RUN---------------------------------------
-----------------------------PRODUCERS---------------------------------------
*/
void* run_producers(int port){
   //declare variables
   int sockfd, client_fd;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   pthread_t tid;

   memset((char *) &serv_addr, 0, sizeof(serv_addr)); //azzera server_addr

   //setup socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0); //crea il socket per il server: flusso bidirezionale, protocollo IPV4
   if (sockfd < 0) {
     perror("ERROR opening socket");
     exit(EXIT_FAILURE); //in caso di errore non è sicuro proseguire
   }


   setup_server(&serv_addr, port); //inizializza sockaddr_in del server

   if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {//assegna l'indirizzo al socket
      perror("ERROR on binding");
      exit(EXIT_FAILURE); //in caso di errore non è sicuro proseguire
    }
   //start socket connection
   listen(sockfd, MAX_CLIENT_QUEUE_REQUEST); //rende il socket pronto a ricevere richieste da parte di client, con queue massima
   clilen = sizeof(cli_addr);
   while (1)
   {
       client_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); //accetta la connessione da un client
       if (client_fd < 0)
           perror("ERROR on accept");
       else if(pthread_create(&tid, NULL, &handle_client, (void*)new_client_info(&cli_addr, client_fd))!=0) //crea un thread indipendete produttore, passando le informazioni sul client attraverso la struct client_info
           perror("Error while making thread");
   }
   close(sockfd);
   return 0;
 }

void* handle_client(void* client_inf){
   client_info* client_info_ = (client_info*)client_inf;
   int client_fd = client_info_->cli_fd;
   char* addr = client_info_->cli_addr;

   //struct linkedlist* local_log;// = new_linkedlist(NULL);//si può modificare mettendo all'inizio un nodo con l'indirizzo+nome
   struct node* node_client_fd;
   struct node* username_node;

   int byte_read, get_name, flag;
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
       //printf("%s\n", name);
       if(check_username_already_taken(name))
       {
         handle_client_name_taken(client_fd);
         flag = 1;
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
       if(flag) break;
     }
     append_string_message_to_send(buffer, byte_read, client_fd, addr);//GLOBAL LOG
   }

   close(client_fd);
   send_goodbye(buffer, name, addr);
   remove_node_client_fd(node_client_fd);
   remove_node_username(username_node);

   free(client_info_->cli_addr);
   free(client_inf);
   return NULL;
 }

//---------------------------COMUNICATION BETWEN PRODUCERS-CONSUMER-------------------
void append_string_message_to_send(char*string, int len, int client_fd, char* addr){
  pthread_mutex_lock(&message_to_send_struct.message_to_send->mutex);
  char* buffer= (char*)calloc(len, sizeof(char)); //allocate in heap the message (so is not lost at the end of function)
  buffer = strncpy(buffer, string, len);

  sender_msg* msg = new_sender_msg(buffer, client_fd, addr);
  struct node* node = new_node((void*)msg);//new message node
  if(message_to_send_struct.message_to_send->lenght) //has next; alias-> there are other messages in queue to be sent
  {                                                       //
    struct node* append_before = NULL;
    struct node* append_node_before = check_youngest_msg(node, message_to_send_struct.message_to_send->first, append_before); //node older than new message

    int inserted = insert_first(append_node_before, node); //new node is add first ONLY if is older than anoter msg in queue
    if(message_to_send_struct.message_to_send->first == append_node_before && inserted)
      message_to_send_struct.message_to_send->first = node;
    else //other msg older than new msg
    {
      //printf("APPENDO ALLA FINE\n");
      append_node(message_to_send_struct.message_to_send, node);//append node in head of list
    }
  }
  else append_node(message_to_send_struct.message_to_send, node); //else there arent msg in queue

  //signal the consumer and release lock
  pthread_cond_signal(&message_to_send_struct.new_message);
  pthread_mutex_unlock(&message_to_send_struct.message_to_send->mutex);
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

 void send_goodbye(char* buffer, char* name, char* addr){
   memset(buffer, 0, BUFFER_SIZE_MESSAGE);
   time_t t = time(NULL);
   struct tm tm = *localtime(&t);
   snprintf(buffer, BUFFER_SIZE_MESSAGE, "%d-%d-%d %d:%d:%d\n%s%s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,"HAS LEFT THE CHATROOM-->", name);

   append_string_message_to_send(buffer, strlen(buffer), -1, addr);//GLOBAL LOG
 }

 /*
  -------------------------------------RUN CONSUMER-------------------------------------
  Il consumatore si occupa di prendere i messaggi appesi alla struct message_to_send
  e mandarli a tutti i client connessi. prende e rimuove sempre il primo messaggio della
  lista(il più vecchio), poi libera l'heap dalle strutture allocate e dai mesasggi e
  prosegue se ci sono messaggi.
 */

void* run_consumer(void* null) {
  int fd = open("logs/global_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0666); //apre il file di testo dove vengono inseriti i log globali in append mode
  if(fd < 0)
    perror("Error while opening global log: ");
  while (1)
  {
    pthread_mutex_lock(&message_to_send_struct.message_to_send->mutex); //fa un lock sui messaggi da mandare per poi mettersi in ascolto sulla condition
    pthread_cond_wait(&message_to_send_struct.new_message, &message_to_send_struct.message_to_send->mutex);//rilascia il lock e si mette in asclto

    pthread_mutex_unlock(&message_to_send_struct.message_to_send->mutex); //all'arrivo du un messaggio crea ina finestra temporale per dare modo ad altri messaggi
    sleep(0.3);                                                   //ritardatari di arrivare e mettersi prima o dopo il messaggio arrivato a seconda del timestamp
    pthread_mutex_lock(&message_to_send_struct.message_to_send->mutex);//riprende il controllo della lock per escludere i produttori ad inserire messaggi

    while(message_to_send_struct.message_to_send->first)//itero sulla linkedlist: while has next, ed è certo avere almeno 1 elemento
    {
      pthread_mutex_lock(&client_fd_linkedlist.mutex); //prende la lock sulla linkedlist dei client_fd per evitare che un client si aggiunga o rimuova durante l'inoltro dei messaggi

      struct node* actual_client_fd_node = client_fd_linkedlist.first; //prendo il puntatore al primo nodo del client_fd
      sender_msg* sender = (sender_msg*)message_to_send_struct.message_to_send->first->value; //estrare il primo struct sender_msg che si trova nel nodo della linkedlist dei messaggi da mandare

      while(actual_client_fd_node)//itero sui client_fd
      {
        /*if false the message has been send by same user so discard. viene ricoperto anche il caso particolare: uno user si disconnette liberando il proprio client_fd, che viene preso da un altro
        user che si connette prendendo l'fd appena liberato, in quel caso se non si facesse check_peer il messaggio non verrebbe inoltrato a questo user perché condivide l'fd del sender, allora guardando
        l'indirizzo associato all'fd si può capire se è lo stesso user che ha mandato il messaggio oppure uno user diverso con lo stesso fd;
        check_peer viene eseguito solo e solo se il l'fd del sender è == all'fd del nodo nella lista all'i-esima iterazione*/
        if(*(int*)actual_client_fd_node->value != sender->sockfd || check_peer(sender->sockfd, sender->addr) != 0)
            write(*(int*)actual_client_fd_node->value, sender->message, BUFFER_SIZE_MESSAGE); //si possono ignorare eventuali errori perché dei client se ne occupa il producer
        actual_client_fd_node = actual_client_fd_node->next;//si aggiorna il nodo successivo
      }//fine iterazione sui client_fd
      store_message_to_send(sender->message, sender->addr, fd); //scrive su disco il messaggio mandato dal client; fd è il file descriptor del file global_log.txt precedentemente aperto
      free(sender->message); //libera l'head dalla stringa messaggio
      pthread_mutex_unlock(&client_fd_linkedlist.mutex);//usciamo dalla sessione critica sbloccando il client_fd_linkedlist per permettere ai produccer di rimuovere o aggiungere client_fd
      remove_first_from_linked_list(message_to_send_struct.message_to_send); /*rimuove il ndoo dall'inizio della lista linkata (con complessità Theta(1)), liberando lo spazio nell'heap
                                                                              per evitare altrimenti un inevitabile saturamento*/
    }
    pthread_mutex_unlock(&message_to_send_struct.message_to_send->mutex); /*rimossi tutti i messaggi dalla linkedlist message_to_send_struct.message_to_send: hasnext == false, si può
                                                                            restituire la linkedlist ai producers che potranno appendere e segnalare altri messaggi*/
  }//si ritorna al while(1) per rimettersi in ascolto sulla condition
  return 0;
}

void store_message_to_send(char* msg, char* cli_address, int fd){
    int byte_w = write(fd, msg, strlen(msg)); //scrive su global_log.txt il messaggio del client (composto da: -timestamp -nome -text)
    if(byte_w < 0)
      perror("Error while writing logs: ");
    byte_w = write(fd, cli_address, BUFFER_IPV4_SIZE); //scrive su global_log.txt l'ip del client che ha mandato il messaggio
    if(byte_w < 0)
      perror("Error while writing logs: ");
    byte_w = write(fd, "\n", 1); //appende su global_log.txt un newline
    if(byte_w < 0)
      perror("Error while writing logs: ");
}


int check_peer(int client_fd, char* addr_sender){
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  getpeername(client_fd, (struct sockaddr *)&addr, &addr_size);//uso questa funzione per prendere l'ip dal fd
  return strcmp(inet_ntoa(addr.sin_addr), addr_sender); //cmp ip del fd con l'ip del sender
}
