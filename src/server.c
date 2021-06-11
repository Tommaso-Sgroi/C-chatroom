#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>//TO REMOVE?
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <execinfo.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "datastructures/dynamiclist.c"
#include "datastructures/size.h"
#include "server_/datastructure/sstructure.c"
#include "headers/server.h"
#define MAX_CLIENT_QUEUE_REQUEST 16 //coda di attesa delle richieste client


int global_log_fd;
arraylist clients_fd_list, clients_usernames;

struct {
    arraylist buffer_messages_list;
    pthread_cond_t new_message;
}buffer_messages;




int main(int argc, char *argv[]) {
  if(argc > 2)
  {
    fprintf(stderr, "Too many arguments, argument must be the port number\n");
    exit(EXIT_FAILURE);
  }
  else if(argc < 2)
  {
    fprintf(stderr, "Port not found, argument must be the port number\n");
    exit(EXIT_FAILURE);
  }
  //signal(SIGSEGV, handler);   // install our handler


  printf("Use \"kill -s SIGUSR1 %d\" for stop the server\n", getpid());
  setup(); //crea le liste linkate, crea le mutex, crea le conditions e crea l'ambiente per i log
  pthread_t tid; //thread id del consumatore

  if(pthread_create(&tid, NULL, &run_consumer, NULL)!=0) //crea il thread consumatore
  {
    perror("Cannot create consumer thread");//se ci sono errori allora non bisogna continuare
    exit(EXIT_FAILURE);
  }

  run_producers(atoi("6000")); //continua l'esecuzione come produttore
}

void setup(){
    //setup_signal_handler();
    setup_arraylist();
    setup_mutex();
    setup_cond();
    setup_logs();
}

int setup_server(struct sockaddr_in* serv_addr, int port){
    serv_addr->sin_addr.s_addr = INADDR_ANY; //permette connessioni da tutti gli indirizzi
    serv_addr->sin_port = htons(port); //imposta la porta
    return 0;
}


  void setup_cond() {
    if (pthread_cond_init(&buffer_messages.new_message, NULL) != 0)//inizializza la condition di arrivo messaggio
    {//se da errore non è sicuro continuare
        perror("Error in pthread_cond_init()\n");
        exit(EXIT_FAILURE);
    }
  }
  // void setup_signal_handler(){
  //   struct sigaction act;
  //   sigset_t set; /* insieme di segnali (simile all'insieme di fd della select...) */
  //
  //   sigemptyset( &set ); /* set = {} */
  //   sigaddset( &set, SIGUSR1 ); /* set = {SIGUSR1} */
  //
  //   act.sa_flags = 0; /* questo campo serve principalmente per chiarire alcuni usi di SIGCHLD,
  //      ma c'e' anche SA_NODEFER, vedere il man  */
  //   act.sa_mask = set; /* i segnali in set sono "masked": se arriveranno mentre l'handler e'
  //       in esecuzione, verranno messi in attesa (nel qual caso, si sta gia'
  //       gestendo un SIGUSR1...) */
  //   act.sa_handler = &sighandler; /* puntatore alla funzione (di cui e' stato dato il prototipo
  //         come prima istruzione del main) */
  //   sigaction( SIGUSR1, &act, NULL );
  //
  //   if(sigaction(SIGINT, &act, NULL) == -1) //aggiungo il signal handler
  //   {//se errore non è sicuro prosegurie
  //     perror("sigaction:");
  //     exit(EXIT_FAILURE);
  //   }
  // }
    void setup_mutex(){

        if(pthread_mutex_init(&buffer_messages.buffer_messages_list.mutex, NULL) != 0 || pthread_mutex_init(&clients_fd_list.mutex, NULL) != 0) //inizializza le mutex per le liste linkate del client_fd e i messaggi da mandare
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

void setup_arraylist(){
    initArray(&clients_fd_list, 10);
    initArray(&clients_usernames, 10);
    initArray(&buffer_messages.buffer_messages_list, 16);
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
   while(1)
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
  //estrae il filedescriptor e l'indirizzo ip del client
  client_info* client_info_ = (client_info*)client_inf;
  int client_fd = client_info_->cli_fd;
  char* addr = client_info_->cli_addr;


  /*
  byte_read: intero che indica il numero di byte letti dal client
  get_name: intero (booleano) che indica se è già stato estratto lo username (viene fatto solo alla lettura del primo messaggio, hello message del clinet)
  flag: intero che indica se il nome è già stato preso
  -
  */
  int byte_read;
  char name[BUFFER_NAME_SIZE];//buffer di memorizzazione del nome utente (viene estratto qui)
  memset(name, 0, BUFFER_NAME_SIZE); //inizializzato a 0 il buffer

  byte_read = read(client_fd, name, BUFFER_NAME_SIZE);
  if(byte_read <= 0 /*&& name already taken*/)
  {
    close(client_fd);
    free(client_inf);
    return NULL;
  }
  
  append_client_fd(client_fd);
  append_username((unsigned long) name);

  char message_buf[BUFFER_SIZE_MESSAGE];
  while(1)
  {
    memset(message_buf, 0, BUFFER_SIZE_MESSAGE);
    byte_read = read(client_fd, message_buf, BUFFER_SIZE_MESSAGE);
    if(byte_read <= 0) break;

    append_string_message_to_send(message_buf, byte_read, client_fd, addr);
  }
  
  remove_client_fd(client_fd); 
  remove_username((unsigned long) name);
  close(client_fd);
  free(client_inf);
  return NULL;
 }

 //---------------------------COMUNICATION BETWEN PRODUCERS-CONSUMER-------------------
void append_string_message_to_send(char* message, int len, int client_fd, char* addr){
  pthread_mutex_lock(&buffer_messages.buffer_messages_list.mutex);//entro nella sessione critica, nessuno deve poter toccare i messaggi

  sender_msg* sender = new_sender_msg(message, client_fd, len, addr);

  if(buffer_messages.buffer_messages_list.used > 0) //has next; alias-> there are other messages in queue to be sent; controlla se ci sono messaggi in coda
  {
    unsigned long index = check_youngest_msg(sender);
    if(index == 0) //allora non è stato torvato un messaggio più giovane
      insertArray(&buffer_messages.buffer_messages_list, (unsigned long) sender); //appende in testa alla lista
    else
      insertAt(&buffer_messages.buffer_messages_list, --index, (unsigned long) sender); //appende all'indice index della lista
  }
  else insertArray(&buffer_messages.buffer_messages_list, (unsigned long) sender); //appende in testa alla lista

  pthread_cond_signal(&buffer_messages.new_message);//segnala al consumer (se è in ascolto) che è arrivato un nuovo messaggio
  pthread_mutex_unlock(&buffer_messages.buffer_messages_list.mutex);//si esce dalla sessione critica
}


unsigned long check_youngest_msg(sender_msg* sender){
  if(sender == NULL) return 0; //controlla se l'input non è nullo

  arraylist* array_list = &buffer_messages.buffer_messages_list;
  timestamp* sender_ts = new_timestamp(sender->message); //crea un oggetto timestamp che contiene il timestamp del sende

  int flag = 0;
  unsigned long index = 0;
  for(unsigned long i = 0; i < array_list->used; i++)
  {
    char* buffer = (char*) array_list->array[i];
    timestamp* other_ts = new_timestamp(buffer); //crea ed estrae il timestamp dell'altro messaggio neklla lista
    if(sender_ts->year < other_ts->year) //se il messaggio del sender è stato mandato un anno precedente a quello del più vecchio posizionati dietro di esso
    {
      ++flag;
      index = i;
    }
    else if(sender_ts->year == other_ts->year && sender_ts->month < other_ts->month) //se il messaggio del sender è stato mandato un anno precedente e lo stesso anno a quello del più vecchio posizionati dietro di esso
    {
      ++flag;
      index = i;
    }
    else if(sender_ts->month == other_ts->month && sender_ts->day < other_ts->day)//se il messaggio del sender è stato mandato un giorno precedente e lo stesso mese a quello del più vecchio posizionati dietro di esso
    {
      ++flag;
      index = i;
    }
    else if(sender_ts->day == other_ts->day && sender_ts->hours < other_ts->hours)//se il messaggio del sender è stato mandato un ora precedente e lo stesso giorno a quello del più vecchio posizionati dietro di esso
    {
      ++flag;
      index = i;
    }
    else if(sender_ts->hours == other_ts->hours && sender_ts->min < other_ts->min)//se il messaggio del sender è stato mandato un minuto precedente e lo stessa ora a quello del più vecchio posizionati dietro di esso
    {
      ++flag;
      index = i;
    }
    else if(sender_ts->min == other_ts->min && sender_ts->sec < other_ts->sec) //se il messaggio del sender è stato mandato un secondo precedente e lo stesso minuto a quello del più vecchio posizionati dietro di esso
    {
      ++flag;
      index = i;
    }
    free(other_ts);//libera il ttimestamp dell'altro
    if(flag) 
      break;
  }
  free(sender_ts);
  return flag? index + 1: 0; /*ritorna il valore dellìindice + 1, questo perché ritorna un unsigned, quindi bisogna tornare 0 come valore da ignorare
  ritorna l'indice in cui inserire il valore + 1*/
}

//------------------------CLIENT LOG-IN------------------------------------


void append_client_fd(int client_fd){
  pthread_mutex_lock(&clients_fd_list.mutex);//si entra nella sezione critica del client_fd, bisogna evitare che cambi qualcosa nel mentre
  insertArray(&clients_fd_list, client_fd); //appende il nodo in fondo alla lista
  pthread_mutex_unlock(&clients_fd_list.mutex);//si esce dalla sessione critica
 }

void append_username(unsigned long username_addr){
  pthread_mutex_lock(&clients_usernames.mutex);//si entra nella sezione critica del client_fd, bisogna evitare che cambi qualcosa nel mentre
  insertArray(&clients_usernames, username_addr); //appende il nodo in fondo alla lista
  pthread_mutex_unlock(&clients_usernames.mutex);//si esce dalla sessione critica
}

 //--------------------USER DISCONNECTED---------------------------------------

//   void remove_node_username(struct node* username){
// //    pthread_mutex_lock(&usernames.mutex);
// //    remove_node_from_linkedlist(username, &usernames); //rimuove lo username dalla lista degli usernamae
// //    pthread_mutex_unlock(&usernames.mutex);
//  }

 void remove_client_fd(int client_fd){
  pthread_mutex_lock(&clients_fd_list.mutex);//si entra nella sezione critica del client_fd, bisogna evitare che cambi qualcosa nel mentre
  removeElement(&clients_fd_list, client_fd); //appende il nodo in fondo alla lista
  pthread_mutex_unlock(&clients_fd_list.mutex);//si esce dalla sessione critica
 }

void remove_username(unsigned long username_addr){
  pthread_mutex_lock(&clients_usernames.mutex);//si entra nella sezione critica del client_fd, bisogna evitare che cambi qualcosa nel mentre
  removeElement(&clients_usernames, username_addr); //appende il nodo in fondo alla lista
  pthread_mutex_unlock(&clients_usernames.mutex);//si esce dalla sessione critica
}

 void send_goodbye(char* buffer, char* name, char* addr){
//    memset(buffer, 0, BUFFER_SIZE_MESSAGE); //azzera il buffer
//    time_t t = time(NULL); //prendi il tempo corrente
//    struct tm tm = *localtime(&t); //estrai il tempo locale
//    //inserisci nel buffer una stringa contnente timestamp + la stringa di avviso agli altri utenti che l'utente x ha lasciato la chatroom + nome utente
//    snprintf(buffer, BUFFER_SIZE_MESSAGE, "%d-%d-%d %d:%d:%d\n%s%s\n ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,"HAS LEFT THE CHATROOM-->", name);
//    printf("Messo nel buffer il messaggio di arrivederci\n");

//    append_string_message_to_send(buffer, strlen(buffer), -1, addr);//appende la stringa nei messaggi da mandare con un fd non valido
//    printf("appeso il messaggio\n");

 }

  /*
  -------------------------------------RUN CONSUMER-------------------------------------
  Il consumatore si occupa di prendere i messaggi appesi alla struct message_to_send
  e mandarli a tutti i client connessi. prende e rimuove sempre il primo messaggio della
  lista(il più vecchio), poi libera l'heap dalle strutture allocate e dai mesasggi e
  prosegue se ci sono messaggi.
 */

void* run_consumer(void* null) {
  global_log_fd = open("logs/global_log.txt", O_WRONLY | O_APPEND | O_CREAT, 0666); //apre il file di testo dove vengono inseriti i log globali in append mode
  if(global_log_fd < 0)
    perror("Error while opening global log: ");
  while (1)
  {
    pthread_mutex_lock(&buffer_messages.buffer_messages_list.mutex); //fa un lock sui messaggi da mandare per poi mettersi in ascolto sulla condition
    if(&buffer_messages.buffer_messages_list.used == 0)//se non sono stati appesi messaggi da quando è stata rilasciata la lock a quando è stata riacquisita allora aspetta
      pthread_cond_wait(&buffer_messages.new_message, &buffer_messages.buffer_messages_list.mutex);//rilascia il lock e si mette in asclto
    /*
    i comandi commentati qui sotto sono serviti a testare l'ordinamento dei messaggi secondo timestamp quando arrivano
    nella stessa finestra temporale
    */
    // pthread_mutex_unlock(&buffer_messages.buffer_messages_list.mutex); //all'arrivo du un messaggio crea ina finestra temporale per dare modo ad altri messaggi
    // sleep(5);                                                   //ritardatari di arrivare e mettersi prima o dopo il messaggio arrivato a seconda del timestamp
    // pthread_mutex_lock(&buffer_messages.buffer_messages_list.mutex);//riprende il controllo della lock per escludere i produttori ad inserire messaggi
    arraylist* messages = &buffer_messages.buffer_messages_list;
    for(unsigned long i = 0; i < buffer_messages.buffer_messages_list.used; i++)
    { 
      pthread_mutex_lock(&clients_fd_list.mutex);
      //prendo il primo messaggio
      sender_msg* msg = (sender_msg*) messages->array[i]; 
 
      int client_fd = -1;
      for(unsigned long n = 0; n < clients_fd_list.used; n++)
      {
        client_fd = (int)clients_fd_list.array[n];
        if(client_fd != msg->sockfd)
          write(client_fd, msg->message, msg->massage_byte); //to improve!!

      }
      pthread_mutex_unlock(&clients_fd_list.mutex);
      store_message_to_send(msg->message, msg->massage_byte, msg->address); //scrive su disco il messaggio mandato dal client; fd è il file descriptor del file global_log.txt precedentemente aperto
    }
    size_t i1 = buffer_messages.buffer_messages_list.used;
    do
    {
      sender_msg* msg = (sender_msg*) messages->array[i1];
      removeElement(&buffer_messages.buffer_messages_list, (unsigned long) msg);
      free(msg);
    } while (i1-- != 0);
    
    pthread_mutex_unlock(&buffer_messages.buffer_messages_list.mutex); //fa un lock sui messaggi da mandare per poi mettersi in ascolto sulla condition
  }//si ritorna al while(1) per rimettersi in ascolto sulla condition

  return 0;
}


void store_message_to_send(char* msg, int byte_message, char* cli_address){

  int byte_w = write(global_log_fd, msg, byte_message); //scrive su global_log.txt il messaggio del client (composto da: -timestamp -nome -text)
  if(byte_w < 0)
    perror("Error while writing logs: ");
  byte_w = write(global_log_fd, cli_address, strlen(cli_address)); //scrive su global_log.txt l'ip del client che ha mandato il messaggio
  if(byte_w < 0)
    perror("Error while writing logs: ");
  byte_w = write(global_log_fd, "\n", 1); //appende su global_log.txt un newline
  if(byte_w < 0)
    perror("Error while writing logs: ");
}


// int check_peer(int client_fd, char* addr_sender){
// //   struct sockaddr_in addr;
// //   socklen_t addr_size = sizeof(struct sockaddr_in);
// //   int success = getpeername(client_fd, (struct sockaddr *)&addr, &addr_size);//uso questa funzione per prendere l'ip dal fd
// //   return success == 0 && strcmp(inet_ntoa(addr.sin_addr), addr_sender) != 0; //compare ip del fd con l'ip del sender
// }