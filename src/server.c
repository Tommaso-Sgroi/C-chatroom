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
//#define users_online_string "Users online--> "

int global_log_fd, server_socket;
arraylist clients_fd_list;

struct {
    arraylist buffer_messages_list; //buffer dei messaggi
    pthread_cond_t new_message; //condition che segnala al consumatore l'arrivo di un messaggio
}buffer_messages;


void signal_handler(){ 
  char good_bye[] = "Server closed!!\nGood bye ;)\n"; 
  int good_bye_len = 30; 

  close(global_log_fd); //chiude il file descriptore del log globale
  int sockfd;
  for(unsigned long index = 0; index < clients_fd_list.used; index++) //itera sui client e gli manda un messaggio di arrivederci per poi chiudere la connessione
  {
    sockfd = (int)clients_fd_list.array[index]; //estrae il file descriptor
    write(sockfd, good_bye, good_bye_len); //manda l'arrivederci al client
    close(sockfd); //chiude la connessione
  }
  
  close(server_socket); //chiude la connessione sul server
  exit(EXIT_SUCCESS);
}

void exit_server(int exit_status){ //uscita di emergenza in seguito a un error
  for(unsigned long index = 0; index < clients_fd_list.used; index++)
    close((int)clients_fd_list.array[index]); //chiude tutti i client file descritor aperti sul server
  
  close(server_socket); //chiude gli altri file descriptor
  close(global_log_fd); //chiude il global log fd
  
  exit(exit_status); //esce con l'exit status passato in input
}

int main(int argc, char *argv[]) {
  if(argc > 3)
  {
    fprintf(stderr, "Too many arguments, argument must be the port number\n");
    exit_server(EXIT_FAILURE);
  }
  else if(argc < 3)
  {
    fprintf(stderr, "Port not found, argument must be the port number\n");
    exit(EXIT_FAILURE);
  }

  printf("Use \"kill -s SIGUSR1 %d\" for stop the server\n", getpid()); //printd il pid con le istruzioni di interruzione del processo
  setup(); //crea le liste linkate, crea le mutex, crea le conditions e crea l'ambiente per i log
  pthread_t tid; //thread id del consumatore

  if(pthread_create(&tid, NULL, &run_consumer, NULL)!=0) //crea il thread consumatore
  {
    perror("Cannot create consumer thread");//se ci sono errori allora non bisogna continuare
    exit_server(EXIT_FAILURE);
  }

  run_producers(atoi(argv[2]), argv[1]); //continua l'esecuzione come produttore
}

void setup(){
  setup_signal_handler(); //setup sul signal handler
  setup_arraylist(); //setup sugli arraylist
  setup_mutex(); //setup sulle mutex
  setup_cond(); //setup sulle conditions
  setup_logs(); //crea l'ambiente per lo storing del global log
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
    exit_server(EXIT_FAILURE);
  }
}

void setup_signal_handler(){
  struct sigaction act;
  sigset_t set; // insieme di segnali (simile all'insieme di fd della select...)

  sigemptyset(&set);//insieme vuoto
  sigaddset(&set, SIGUSR1); //inizializzo l'insieme

  act.sa_flags = 0;
  act.sa_mask = set; //crea la maschera dei segnali
  act.sa_handler = &signal_handler; // puntatore alla funzione handler
  sigaction(SIGUSR1, &act, NULL);

  if(sigaction(SIGINT, &act, NULL) == -1) //aggiungo il signal handler
  {//se errore non è sicuro prosegurie
    perror("sigaction:");
    exit_server(EXIT_FAILURE);
  }
}

void setup_mutex(){
  if(pthread_mutex_init(&buffer_messages.buffer_messages_list.mutex, NULL) != 0 || pthread_mutex_init(&clients_fd_list.mutex, NULL) != 0) //inizializza le mutex per le liste linkate del client_fd e i messaggi da mandare
  { //se uno dei 2 da errore allora non è sicuro avviare il server
    fprintf(stderr, "Error in pthread_mutex_init()\n");
    exit_server(EXIT_FAILURE);
  }
}

void setup_logs(){
  DIR* dir = opendir("logs");/*fa un check per vedere se esiste la dir dei log provando ad aprirla*/
  if(dir) closedir(dir); //se esiste allora chiudila perché non serve tenerla aperta
  else if(ENOENT == errno)//altrimenti sc'è l'errore di apertura...
  {
    int r = mkdir("logs", 0770 ); //...crea la dir...
    if(r < 0) perror("Cannot create /log dir");
  }
  else
  {//altrimenti l'errore è dato da qualche altro motivo e non è sicuro andare avanti (si esce)
    perror("Impossibile creare dir logs");
    exit_server(EXIT_FAILURE);
  }
}

void setup_arraylist(){
  /*
  inizializza gli arraylist con una dimensione
  specificata
  */
  initArray(&clients_fd_list, 10);
  initArray(&buffer_messages.buffer_messages_list, 16);
}


  /*
-----------------------------RUN---------------------------------------
-----------------------------PRODUCERS---------------------------------------
*/

void* run_producers(int port, char* serv_address){
  //declare variables
  int client_fd;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  pthread_t tid;

  memset((char *) &serv_addr, 0, sizeof(serv_addr)); //azzera server_addr

  //setup socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0); //crea il socket per il server: flusso bidirezionale, protocollo IPV4
  if (server_socket < 0) {
    perror("ERROR opening socket");
    exit_server(EXIT_FAILURE); //in caso di errore non è sicuro proseguire
  }

  if(inet_pton(AF_INET, serv_address, &(serv_addr.sin_addr)) != 1){
    perror("ERROR hostname");
    exit_server(EXIT_FAILURE);
  }

  setup_server(&serv_addr, port); //inizializza sockaddr_in del server

  if(bind(server_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {//assegna l'indirizzo al socket
    perror("ERROR on binding");
    exit_server(EXIT_FAILURE); //in caso di errore non è sicuro proseguire
  }
  //start socket connection
  if(listen(server_socket, MAX_CLIENT_QUEUE_REQUEST) < 0)//rende il socket pronto a ricevere richieste da parte di client, con queue massima
  {
    perror("Error during listening");
    exit_server(EXIT_FAILURE);
  }


  clilen = sizeof(cli_addr);
  while(1)
  {
    client_fd = accept(server_socket, (struct sockaddr *) &cli_addr, &clilen); //accetta la connessione da un client
    if (client_fd < 0) //errore
      perror("ERROR on accept");
    else if(pthread_create(&tid, NULL, &handle_client, (void*)new_client_info(&cli_addr, client_fd))!=0) //crea un thread indipendete produttore, passando le informazioni sul client attraverso la struct client_info
      perror("Error while making thread");
    memset(&cli_addr, 0, sizeof(struct sockaddr_in));//azzera il client address
  }
  close(server_socket);
  return 0;
 }

void* handle_client(void* client_inf){
  client_info* client_info_ = (client_info*)client_inf; //estrae il pointer della struttura dati passata in input
  int client_fd = client_info_->cli_fd; //estrae il file descriptor
  char* addr = client_info_->cli_addr; //estrare l'indirizzo del client


  /*
  byte_read: intero che indica il numero di byte letti dal client
  get_name: intero (booleano) che indica se è già stato estratto lo username (viene fatto solo alla lettura del primo messaggio, hello message del clinet)
  flag: intero che indica se il nome è già stato preso
  -
  */
  int byte_read; //byte letti dal client
  char message_buf[BUFFER_SIZE_MESSAGE]; //buffer del messaggio, contine il messaggio inviato dal client
  
  if(send_users_online(client_fd, message_buf)) //manda gli utenti online al client e la funzione ritorna 1 in caso di errore
  {
    close(client_fd);
    free(client_inf);
    return NULL;
  }

  append_client_fd(client_fd); //appende il coda il client_fd

  while(1)
  {
    memset(message_buf, 0, BUFFER_SIZE_MESSAGE);//azzera il messaggio
    byte_read = read(client_fd, message_buf, BUFFER_SIZE_MESSAGE); //legge nel buffer il messaggio del client
    if(byte_read <= 0) break; //se errore (il client si è disconnesso) esce dal ciclo

    append_string_message_to_send(message_buf, byte_read, client_fd, addr); //appende il messaggio al buffer del consumatore
  }
  
  remove_client_fd(client_fd); //rimuove il file descriptor dalla lista
  close(client_fd); //chiude il file descriptor
  free(client_inf); //libera la struttura di client info
  return NULL;
 }

 //---------------------------COMUNICATION BETWEN PRODUCERS-CONSUMER-------------------
void append_string_message_to_send(char* message, int len, int client_fd, char* addr){
  pthread_mutex_lock(&buffer_messages.buffer_messages_list.mutex);//entro nella sessione critica, nessuno deve poter toccare i messaggi

  sender_msg* sender = new_sender_msg(message, client_fd, len, addr); //crea un nuovo oggetto sender, serve per far estrarre al consumatore le info utili

  if(buffer_messages.buffer_messages_list.used > 0) //has next; alias-> there are other messages in queue to be sent; controlla se ci sono messaggi in coda
  {
    unsigned long index = check_youngest_msg(sender); // ritorna l'indice dove inserire il messaggio + 1, quindi se == 0 allora non vi è stato trovato un messaggio più vecchio
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

int send_users_online(int client_fd, char* message_buf){
  memset(message_buf, 0, BUFFER_MESSAGE); //resetta il message buffer
  
  pthread_mutex_lock(&clients_fd_list.mutex);
  int success = snprintf(message_buf, BUFFER_SIZE_MESSAGE, "Users online--> %lu", clients_fd_list.used);/*riutilizza il buffer del messaggio per avvisare il client
    del numero di utenti collegati*/
  pthread_mutex_unlock(&clients_fd_list.mutex);

  return success < 0 || write(client_fd, message_buf, strlen(message_buf)) <= 0; //scrive sul client e ritorna il successo o meno
}

 //--------------------USER DISCONNECTED---------------------------------------

 void remove_client_fd(int client_fd){
  pthread_mutex_lock(&clients_fd_list.mutex);//si entra nella sezione critica del client_fd, bisogna evitare che cambi qualcosa nel mentre
  removeElement(&clients_fd_list, client_fd); //appende il nodo in fondo alla lista
  pthread_mutex_unlock(&clients_fd_list.mutex);//si esce dalla sessione critica
 }


  /*
  -------------------------------------RUN-------------------------------------
  Il consumatore si occupa di prendere i messaggi appesi alla struct message_to_send
  e mandarli a tutti i client connessi. prende e rimuove sempre il primo messaggio della
  lista(il più vecchio), poi libera l'heap dalle strutture allocate e dai mesasggi e
  prosegue se ci sono messaggi.
 */

void* run_consumer(void* null) {
  global_log_fd = open("logs/global_log.txt", O_WRONLY | O_APPEND | O_CREAT | O_SYNC, 0660); //apre il file di testo dove vengono inseriti i log globali in append mode
  if(global_log_fd < 0)
    perror("Error while opening global log: ");
  while(1)
  {
    pthread_mutex_lock(&buffer_messages.buffer_messages_list.mutex); //fa un lock sui messaggi da mandare per poi mettersi in ascolto sulla condition
    if(buffer_messages.buffer_messages_list.used == 0)//se non sono stati appesi messaggi da quando è stata rilasciata la lock a quando è stata riacquisita allora aspetta
      pthread_cond_wait(&buffer_messages.new_message, &buffer_messages.buffer_messages_list.mutex);//rilascia il lock e si mette in asclto
    /*
    i comandi commentati qui sotto sono serviti a testare l'ordinamento dei messaggi secondo timestamp quando arrivano
    nella stessa finestra temporale
    */
    // pthread_mutex_unlock(&buffer_messages.buffer_messages_list.mutex); //all'arrivo du un messaggio crea ina finestra temporale per dare modo ad altri messaggi
    // sleep(5);                                                   //ritardatari di arrivare e mettersi prima o dopo il messaggio arrivato a seconda del timestamp
    // pthread_mutex_lock(&buffer_messages.buffer_messages_list.mutex);//riprende il controllo della lock per escludere i produttori ad inserire messaggi
    arraylist* messages = &buffer_messages.buffer_messages_list;
    for(unsigned long i = 0; i < buffer_messages.buffer_messages_list.used; i++) //per ogni messaggio nel buffer
    { 
      pthread_mutex_lock(&clients_fd_list.mutex); //metti la lock sui client fd
      //prendo il primo messaggio
      sender_msg* msg = (sender_msg*) messages->array[i]; //estrai la struttura del mittente
 
      int client_fd = -1; //crea un file descriptor obsoleto che verrà rimpiazzato all'iterazione successiva
      for(unsigned long n = 0; n < clients_fd_list.used; n++) //per ogni file descriptor
      {
        client_fd = (int)clients_fd_list.array[n]; //estrai il file descriptor
        if(client_fd != msg->sockfd) //se il file descriptor non coincide con quello del sender allora non è stato lui a mandare il messaggio
          write(client_fd, msg->message, msg->massage_byte); /*non c'è bisogno di controllare eventuali errori perché in caso sarà il thread
          al produttore (client) a occuparsi di chiudere la connessione o gestire l'errore*/
      }
      pthread_mutex_unlock(&clients_fd_list.mutex); //sblocca la lista del file descriptor
      store_message_to_send(msg->message, msg->massage_byte, msg->address); //scrive su disco il messaggio mandato dal client; fd è il file descriptor del file global_log.txt precedentemente aperto
    }
    
    do //bisogna fare pulizia del buffer, se si è arrivati qui vuol dire che almeno un messaggio è presente
    {
      sender_msg* msg = (sender_msg*) messages->array[messages->used -1]; //estrare la struttura sender_msg
      removeElement(messages, (unsigned long) msg); //rimuove l'elemento che corrisponde alla locazione in memoria del messaggio
      free(msg); //libera la struttura
    } while(messages->used > 0);
     
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