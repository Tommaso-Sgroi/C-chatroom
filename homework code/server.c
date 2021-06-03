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
#include "datastructure/linkedlist.c"
#include "server_/server.h"
#include "datastructure/size.h"
#define MAX_CLIENT_QUEUE_REQUEST 16 //coda di attesa delle richieste client



struct linkedlist client_fd_linkedlist, usernames; //linkedlist per contenere client_fd e gli usernames in modo dinamico

struct {
  struct linkedlist* message_to_send; //linkedlist dei messaggi che verranno mandati dal consumer
  //struct node* last_read;
  pthread_cond_t new_message; //segnala al thread consumatore che è arrivato un nuovo messaggio
} message_to_send_struct; //serve solo una unica istanza di questa struttura in cui vengono inseriti i messaggi in arrivo ordinati secondo timestamp



void sighandler(int signo)
{
	if(signo == SIGUSR1) //intercetta il segnale SIGUSR1
  {
    pthread_mutex_lock(&client_fd_linkedlist.mutex); //sezione critica del client_fd, non deve cambiare durante l'iterazione per evitare di leggere da aree di memoria altrimenti invalide

    struct node *clifd = client_fd_linkedlist.first; //primo nodo contente il client_fd
    int sockfd; //qui è dove verrà estratto l'fd del client
    char server_close[] = "Server closed!\nGoodbye!\n"; //stringa di avviso della chiusura del server
    int len = strlen(server_close); //lunghezza
    while(clifd) //itero sui client fd
    {
      sockfd = *(int*)clifd->value; //valore del clientfd
      write(sockfd, server_close, len); //scrivo sul client_fd senza preoccuparsi degli errori (tanto il server sta venendo chiuso)
      shutdown(*(int*)clifd->value, SHUT_RDWR); //chiude la socket in letttura e scrittura
      clifd = clifd->next; //prossimo nodo
    }
    exit(EXIT_SUCCESS); //esce esce da tutti i thread con successo
  }
}

/*
-----------------------------MAIN---------------------------------------
*/
int main(int argc, char *argv[]) {
  // if(argc > 2)
  // {
  //   fprintf(stderr, "Too many arguments, argument must be the port number\n");
  //   exit(EXIT_FAILURE);
  // }
  // else if(argc < 2)
  // {
  //   fprintf(stderr, "Port not found, argument must be the port number\n");
  //   exit(EXIT_FAILURE);
  // }
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
    setup_signal_handler();
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

  void setup_signal_handler(){
    struct sigaction act;
    sigset_t set; /* insieme di segnali (simile all'insieme di fd della select...) */

    sigemptyset( &set ); /* set = {} */
    sigaddset( &set, SIGUSR1 ); /* set = {SIGUSR1} */

    act.sa_flags = 0; /* questo campo serve principalmente per chiarire alcuni usi di SIGCHLD,
       ma c'e' anche SA_NODEFER, vedere il man  */
    act.sa_mask = set; /* i segnali in set sono "masked": se arriveranno mentre l'handler e'
        in esecuzione, verranno messi in attesa (nel qual caso, si sta gia'
        gestendo un SIGUSR1...) */
    act.sa_handler = &sighandler; /* puntatore alla funzione (di cui e' stato dato il prototipo
          come prima istruzione del main) */
    sigaction( SIGUSR1, &act, NULL );

    if(sigaction(SIGINT, &act, NULL) == -1) //aggiungo il signal handler
    {//se errore non è sicuro prosegurie
      perror("sigaction:");
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
  //estrae il filedescriptor e l'indirizzo ip del client
   client_info* client_info_ = (client_info*)client_inf;
   int client_fd = client_info_->cli_fd;
   char* addr = client_info_->cli_addr;

   struct node* node_client_fd; //nodo del client_fd;
   struct node* username_node; /*nodo dello username,  bisogna appendere il nodo
   nella lista così da poter avere un identificatore unico con il client nella seguente
   sessione*/

   /*
   byte_read: intero che indica il numero di byte letti dal client
   get_name: intero (booleano) che indica se è già stato estratto lo username (viene fatto solo alla lettura del primo messaggio, hello message del clinet)
   flag: intero che indica se il nome è già stato preso
    -
   */
   int byte_read, get_name, flag;
   char name[BUFFER_NAME_SIZE + 1];//buffer di memorizzazione del nome utente (viene estratto qui)
   memset(name, 0, BUFFER_NAME_SIZE + 1); //inizializzato a 0 il buffer

   char buffer[BUFFER_SIZE_MESSAGE + 1];//buffer di memorizzazione dell'intero messaggio del client

   while(1)
   {
     memset(buffer, 0, BUFFER_SIZE_MESSAGE + 1);//ogni iterazione viene resettato il buffer del mesasggio
     byte_read = read(client_fd ,buffer, BUFFER_SIZE_MESSAGE); //rimane in ascolto sul client per messaggi
     if(byte_read <= 0) break; //user disconnected or other error occurred, sot safe cont

     if(get_name == 0)
     {
       get_username(buffer, name); //estrae lo username
       printf("preso username ! %s\n", name);
       int f = check_username_already_taken(name);
       printf("username taken? %i\n", f);
       if(f) //if username is taken
       {
         handle_client_name_taken(client_fd); //si occupa di informare il client che il nome utente è già stato preso e chiude la connessione
         flag = 1; //serve per informare che bisogna o non bisogna fare determinate azioni successivamente
       }
       else //else if lo username è disponibile
       {
         username_node = new_node(name); //crea un nodo con all'interno il nome dello username

         printf("%s\n","appendo un nodo con il nome dello username" );
         pthread_mutex_lock(&usernames.mutex); //entro in una zona critica, bisogna evitare che la lista cambi durante l'iterazione
         append_node(&usernames, username_node); //appendo il nodo nella lista
         pthread_mutex_unlock(&usernames.mutex);//esco dalla zona critica

         printf("%s\n","appendo un nodo con il client fd" );
         node_client_fd = append_node_client_fd(&client_info_->cli_fd); //appendo il nodo dei client_fd alla lista, vedere funzione ->
         //local_log = new_linkedlist(NULL);
       }
       if(flag) break; //if il nome è già stato preso allora esci dal while che ormai la connessione è chiusa
       get_name++; //incrementa l'intero per inidcare che il nome dello user è già stato estratto e di non tornare qui
     }
     printf("appendo il nodo del messaggio\n");
     append_string_message_to_send(buffer, byte_read + 1, client_fd, addr);/* se non ci sono stati problemi allora si può informare tutti gli altri client della presenza del nuovo
     arrivato appendendo il messaggio di Hello alla coda dei messaggi da inviare*/
   }

   if(flag == 0) //se il flag è == 0 allora la connsessione del client non è stata rifiutata per il nome è si può proseguire con la rimozione delle strutture dati generate
   {
     printf("%s\n", "chiudo la connessione");
     close(client_fd); //chiude la connessione
     printf("%s\n", "mando arrivederci");
     send_goodbye(buffer, name, addr); //appende un messaggio di arrivederci tra i messaggi da mandare
     printf("%s\n", "rimuovo il nodo dalla lista");
     remove_node_client_fd(node_client_fd); //rimuove il client_fd dalla lista
     printf("%s\n", "rimuovo il nodo dello username");
     remove_node_username(username_node); //rimuovi il nodo dello username dalla lista per liberare il nome per un altro utente
   }

   printf("%s\n", "libero lo spazio cli_addr");
   free(client_info_->cli_addr); //libera dall'heap il client address
   printf("%s\n", "libero lo spazio client_inf");
   free(client_inf); //libera dall'heap la struttura client_info
   printf("\n");
   return NULL;
 }

//---------------------------COMUNICATION BETWEN PRODUCERS-CONSUMER-------------------
void append_string_message_to_send(char*string, int len, int client_fd, char* addr){
  pthread_mutex_lock(&message_to_send_struct.message_to_send->mutex);//entro nella sessione critica, nessuno deve poter toccare i messaggi
  char* buffer = (char*)calloc(len, sizeof(char)); //allocate in heap the message (so is not lost at the end of function)
  printf("%s\n", "copio nell'heap il messaggio dello user");
  buffer = strncpy(buffer, string, len);//copio il messaggio, che si trova nello stack, nell'heap

  printf("Creo una struttura sender msg");
  fflush(stdout);
  sender_msg* msg = new_sender_msg(buffer, client_fd, addr);//creo una struct sender_msg, che contiene il buffer del mesaggio nell'heap, il client_fd e l'indirizzo del mittente
  printf("%s\n", "inserisco la struttura in un nodo");
  struct node* node = new_node((void*)msg);//new message node
  if(message_to_send_struct.message_to_send->lenght) //has next; alias-> there are other messages in queue to be sent; controlla se ci sono messaggi in coda
  {
    struct node* append_node_before = check_youngest_msg(node, message_to_send_struct.message_to_send->first);/*controlla se ci sta un nodo più giovane di quello mandato,
      in caso ritorna il puntatore al nodo dove deve piazzarsi dietro*/

    int inserted = insert_first(append_node_before, node); //new node is add first ONLY if is older than anoter msg in queue, infatti append_node_before è NULL se non vi sono messaggi dietro cui posizionarsi
    if(inserted && message_to_send_struct.message_to_send->first == append_node_before) //if è stato inserito il messaggio e append_node_before è il primo elemento nella coda (quindi abbiamo inserito il nuovo nodo all'inizio della lista)
      message_to_send_struct.message_to_send->first = node; //allora il nuovo nodo diventa il primo elemento della lista
    else //non ci sono messaggi più giovani e va inserito alla fine della lista
      append_node(message_to_send_struct.message_to_send, node);//append node in head of list
  }
  else append_node(message_to_send_struct.message_to_send, node); //else there arent msg in queue, quindi appende il nodo della lista in testa alla coda

  pthread_cond_signal(&message_to_send_struct.new_message);//segnala al consumer (se è in ascolto) che è arrivato un nuovo messaggio
  pthread_mutex_unlock(&message_to_send_struct.message_to_send->mutex);//si esce dalla sessione critica
}

struct node* check_youngest_msg(struct node* node, struct node* other){
  if(node == NULL || other == NULL || node == other) return NULL; /*controlla se gli input non sono nulli,
   può essere nullo quando la coda dei messaggi da inviare sono vuoti! (il primo elemento della lista è NULL)
   ritorna NULL per indicare che non vi sono messaggi dove posizionarsi dietro*/
  sender_msg *sender = (sender_msg*) node->value; //estrae il sender del messsaggio da inviare
  sender_msg *other_sender = (sender_msg*) other->value; /*estrae il sender del primo messaggio della lista
  (che è per firza il più vecchio dei messaggi in quanto tutti vanno a mettersi nella posizione corretta)*/

  struct node* append_before; //puntatore al nodo dietro cui posizionarsi
  struct node* actual_node = other; //nodo della lista da controllare

	timestamp* sender_ts = new_timestamp(sender->message); //crea un oggetto timestamp che contiene il timestamp del sender
  while(actual_node) //itera sui nodi della lista
  {
		timestamp* other_ts = new_timestamp(other_sender->message); //crea ed estrae il timestamp dell'altro messaggio neklla lista
    if(sender_ts->year < other_ts->year) //se il messaggio del sender è stato mandato un anno precedente a quello del più vecchio posizionati dietro di esso
		{
      append_before = actual_node;
      break;
    }
    else if(sender_ts->year == other_ts->year && sender_ts->month < other_ts->month) //se il messaggio del sender è stato mandato un anno precedente e lo stesso anno a quello del più vecchio posizionati dietro di esso
    {
      append_before = actual_node;
      break;
    }
    else if(sender_ts->month == other_ts->month && sender_ts->day < other_ts->day)//se il messaggio del sender è stato mandato un giorno precedente e lo stesso mese a quello del più vecchio posizionati dietro di esso
    {
      append_before = actual_node;
      break;
    }
    else if(sender_ts->day == other_ts->day && sender_ts->hours < other_ts->hours)//se il messaggio del sender è stato mandato un ora precedente e lo stesso giorno a quello del più vecchio posizionati dietro di esso
    {
      append_before = actual_node;
      break;
    }
    else if(sender_ts->hours == other_ts->hours && sender_ts->min < other_ts->min)//se il messaggio del sender è stato mandato un minuto precedente e lo stessa ora a quello del più vecchio posizionati dietro di esso
    {
      append_before = actual_node;
      break;
    }
    else if(sender_ts->min == other_ts->min && sender_ts->sec < other_ts->sec) //se il messaggio del sender è stato mandato un secondo precedente e lo stesso minuto a quello del più vecchio posizionati dietro di esso
    {
      append_before = actual_node;
      break;
    }
    free(other_ts);//libera il ttimestamp dell'altro
    actual_node = actual_node->next; //prossimo elemento
  }
  free(sender_ts);//libera il timestamp del sender
  return append_before;//ritorna il puntatore del nodo dietro cui posizionarsi, NULL se il messaggio è il più giovane
}


//------------------------CLIENT LOG-IN------------------------------------

/*
Parse username from hello message
*/
 void get_username(char*message, char* name){
   int flag; //indice di iterazione su name (dove va copiato il nome)
   unsigned long int len = strlen(message);
   for(unsigned long int i = 0; i<len; i++) //itero dall'inizio alla fine del messaggio
     if(message[i]=='>')//faccio un check per vedere se il char è == a '>' perché il messaggio di Hello è particolare e ha una stringa che indica il nome "has joined -->"
     {                 //così arrivati a '>' so che fino al prossimo '\n' ci saranno solo caratteri appartenenti allo username !
        do name[flag++] = message[++i]; //post incrementa l'indice di dove inserire il carattere dal messaggio nel nome (perché si parte da 0); preincrementa l'indice da cui prendere i caratteri del messaggio
        while(message[i] != '\n' && i < len);//questo perché attualmente ci si trova al carattere '>' che non può far parte del nome, così con in preincremento saltiamo subito alla posizione successiva
        /*riaguardo al while si continua l'iterazione fino allo '\n' dove finisce il nome dello user oppure finché si rimane sotto la larghezza della lunghezza dek messaggio*/
     }
 }

 int check_username_already_taken(char* name){
   /*si itera sulla linkedlist degli username confrontando del stringhe finché
   non se ne trovano 2 uguali oppure non ci sono altre stringhe da confrontare.
   a quel punto ritorna 1 se lo username è già stato preso, altrimenti 0
   */
   printf("itero sugli usernames per confrontare i nomi\n");
   pthread_mutex_lock(&usernames.mutex);

   struct node* actual_node = usernames.first;
   while(actual_node)
     if(strcmp(name, (char*)actual_node->value) == 0)
      return 1;
     else actual_node = actual_node->next;//prossimo elemento su cui iterare, null se ci si trova alla fine

   pthread_mutex_unlock(&usernames.mutex);
   return 0;
 }

 void handle_client_name_taken(int client_fd){
   static char name_taken[] = "Name already taken\n";
   printf("%s\n","avviso il client del nome preso" );
   write(client_fd, name_taken, strlen(name_taken));//manda un messaggio al client per informarlo che il nome utente scelto è già stato preso
   close(client_fd);//chiude la connessione con il client
 }

 struct node* append_node_client_fd(int* client_fd){
   struct node* node = new_node((void*)client_fd); //crea il nodo con il puntatore all'intero che rappresenta il client_fd

   pthread_mutex_lock(&client_fd_linkedlist.mutex);//si entra nella sezione critica del client_fd, bisogna evitare che cambi qualcosa nel mentre
   append_node(&client_fd_linkedlist, node); //appende il nodo in fondo alla lista
   pthread_mutex_unlock(&client_fd_linkedlist.mutex);//si esce dalla sessione critica
   return node; //ritorna il puntatore al nodo (che si trova nell'heap)
 }

//--------------------USER DISCONNECTED---------------------------------------
 void remove_node_username(struct node* username){
   pthread_mutex_lock(&usernames.mutex);
   remove_node_from_linkedlist(username, &usernames); //rimuove lo username dalla lista degli usernamae
   pthread_mutex_unlock(&usernames.mutex);
 }

 void remove_node_client_fd(struct node* client_fd){
   pthread_mutex_lock(&client_fd_linkedlist.mutex); //entra nella zona critia
   remove_node_from_linkedlist(client_fd, &client_fd_linkedlist);//rimuove il nodo dalla lista
   pthread_mutex_unlock(&client_fd_linkedlist.mutex);//esce dalla zona critica
 }

 void send_goodbye(char* buffer, char* name, char* addr){
   memset(buffer, 0, BUFFER_SIZE_MESSAGE); //azzera il buffer
   time_t t = time(NULL); //prendi il tempo corrente
   struct tm tm = *localtime(&t); //estrai il tempo locale
   //inserisci nel buffer una stringa contnente timestamp + la stringa di avviso agli altri utenti che l'utente x ha lasciato la chatroom + nome utente
   snprintf(buffer, BUFFER_SIZE_MESSAGE, "%d-%d-%d %d:%d:%d\n%s%s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,"HAS LEFT THE CHATROOM-->", name);

   append_string_message_to_send(buffer, strlen(buffer), -1, addr);//appende la stringa nei messaggi da mandare con un fd non valido
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

    /*
    i comandi commentati qui sotto sono serviti a testare l'ordinamento dei messaggi secondo timestamp quando arrivano
    nella stessa finestra temporale
    */
    // pthread_mutex_unlock(&message_to_send_struct.message_to_send->mutex); //all'arrivo du un messaggio crea ina finestra temporale per dare modo ad altri messaggi
    // sleep(5);                                                   //ritardatari di arrivare e mettersi prima o dopo il messaggio arrivato a seconda del timestamp
    // pthread_mutex_lock(&message_to_send_struct.message_to_send->mutex);//riprende il controllo della lock per escludere i produttori ad inserire messaggi

    while(message_to_send_struct.message_to_send->first)//itero sulla linkedlist: while has next, ed è certo avere almeno 1 elemento
    {
      pthread_mutex_lock(&client_fd_linkedlist.mutex); //prende la lock sulla linkedlist dei client_fd per evitare che un client si aggiunga o rimuova durante l'inoltro dei messaggi
      printf("Prendo il nodo del primo messaggio\n");
      struct node* actual_client_fd_node = client_fd_linkedlist.first; //prendo il puntatore al primo nodo del client_fd
      printf("estraggo il sender\n");
      sender_msg* sender = (sender_msg*)message_to_send_struct.message_to_send->first->value; //estrare il primo struct sender_msg che si trova nel nodo della linkedlist dei messaggi da mandare

      while(actual_client_fd_node)//itero sui client_fd
      {
        /*if false the message has been send by same user so discard. viene ricoperto anche il caso particolare: uno user si disconnette liberando il proprio client_fd, che viene preso da un altro
        user che si connette prendendo l'fd appena liberato, in quel caso se non si facesse check_peer il messaggio non verrebbe inoltrato a questo user perché condivide l'fd del sender, allora guardando
        l'indirizzo associato all'fd si può capire se è lo stesso user che ha mandato il messaggio oppure uno user diverso con lo stesso fd;
        check_peer viene eseguito solo e solo se il l'fd del sender è == all'fd del nodo nella lista all'i-esima iterazione*/
        printf("Writing %s ", sender->message);
        if(*(int*)actual_client_fd_node->value != sender->sockfd || check_peer(sender->sockfd, sender->addr) != 0)
          {
            printf("on %i\n",  *(int*)actual_client_fd_node->value);
            write(*(int*)actual_client_fd_node->value, sender->message, BUFFER_SIZE_MESSAGE); //si possono ignorare eventuali errori perché dei client se ne occupa il producer (dell'interrompere la connessione)
          }
        actual_client_fd_node = actual_client_fd_node->next;//si aggiorna il nodo successivo
      }//fine iterazione sui client_fd
      store_message_to_send(sender->message, sender->addr, fd); //scrive su disco il messaggio mandato dal client; fd è il file descriptor del file global_log.txt precedentemente aperto

      pthread_mutex_unlock(&client_fd_linkedlist.mutex);//usciamo dalla sessione critica sbloccando il client_fd_linkedlist per permettere ai produccer di rimuovere o aggiungere client_fd
      printf("libero lo spazio del sender\n");
      free(sender->addr); //libera l'address copiato nell'heap
      free(sender->message); //libera la stringa messaggio che si trovava nell'heap per essere condivisa
      free(sender);
      printf("rimuovo il primo elemento dai messaggi\n");
      remove_first_from_linked_list(message_to_send_struct.message_to_send); /*rimuove il ndoo dall'inizio della lista linkata (con complessità Theta(1)), liberando lo spazio nell'heap
                                                                              per evitare altrimenti un inevitabile saturamento*/
      printf("rimosso l'elemento\n");
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
  int success = getpeername(client_fd, (struct sockaddr *)&addr, &addr_size);//uso questa funzione per prendere l'ip dal fd
  return success == 0 && strcmp(inet_ntoa(addr.sin_addr), addr_sender) != 0; //compare ip del fd con l'ip del sender
}
