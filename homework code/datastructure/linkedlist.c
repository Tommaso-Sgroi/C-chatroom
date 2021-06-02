#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

/*
Nodo contenente il puntatore al nodo succesivo, precedente e al valore contenuto
*/
struct node {
  struct node* next;
  struct node* prev;
  void* value;
};

/*
Fa un wrapping del comportamento dei nodi mantenendo la lunghezza, il primo elemento, e l'ultimo elemento.
in oltre contiene di default una mutex, NON INIZIALIZZATA in caso servisse di gestire accessi concorrenti
*/
struct linkedlist {
  struct node* first;
  struct node* last;
  long long int lenght;

  pthread_mutex_t mutex;
};

//NODE
//UTILITY FUNCTIONS
int equals(struct node* node, struct node* node1){
  return node == node1 || node->value == node1->value; //controlla se i puntatori dei nodi sono uguali oppure se il puntatore al valore è lo stesso
}

int insert_first(struct node* first, struct node* insert){
  /*
  inserisce il nodo inser una posizione PRIMA edl nodo first
  insert: nodo da inserire
  first: nodo in cui inserire prima
  */
  if(first == insert || first == NULL || insert == NULL) return 0; //controllo se gli input sono corretti
  insert->next = first; //l'elemento successivo del nodo da inserire diventa il nodo successivo
  insert-> prev = first->prev; //l'elemento precedente del nodo da inserire diventa l'elemento precedente del nodo first
  if(first->prev != NULL) //bisogna conttrollare che il nodo first non sia il primo della lista
    first->prev->next = insert; //in quel caso il prossimo del precedente del nodo in cui inserire prima diventa insert
  first->prev = insert; //il precedente del nodo first diventa insert
  return 1; //ritorna successo
}

int append_last(struct node* last, struct node* insert){
  /*
  inserisce il nodo insert dopo il nodo last
  */
  if(last == NULL || insert == NULL) return 1; //ritorna 1 se erroe
  insert->prev = last; //il precedente insert diventa last (gli si mette davanti)
  if(last->next)
    insert-> next = last->next; //il prossimo di insert diventa nullo (è in cima ai nodi)
  last->next = insert;
  return 0;
}

int remove_node(struct node* first, struct node* node_to_remove){ //cerca  e rimuove il nodo a partire da un nodo scorrendolo verso destra
  while(first)//while node != NULL, aren't at the end
  {
    if(equals(first, node_to_remove)) //check nodes are the same
    {
      //save prev and next
      struct node* prev = first->prev;
      struct node* next = first->next;

      //free(first->value);//could be allocated in heap
      free(first);//deallocate space
      //relink prev and next
      if(prev!=NULL) prev->next = next;
      if(next!=NULL) next->prev = prev;
      return 1; //return success
    }
    first = first->next; //go for next node
    }
  return 0; //return insuccess
}


//INSTANTIATION FUNCTIONS
struct node* new_node(void* value){ //crea un nuovo nodo con i valori inizializzati a null
  struct node *node = (struct node*)malloc(sizeof(struct node)); //alloca nell'heap
  (*node).next = NULL;
  (*node).prev = NULL;
  (*node).value = value;
  return node; //ritorna il pointer
}

struct node* new_node_params(struct node* prev, struct node* next, void* value){
  //initialize node with next and prev nodes
  struct node *node = new_node(value);
  (*node).next = next;
  (*node).prev = prev;
  (*node).value = value;
  //correct next value of previous node
  (*prev).next = node;
  //correct previous value of next node
  (*next).prev = node;
  //-----------------------------------
  return node;
}
//---------------------------------------------------

//LINKED LIST FUNCTIONS

void initialize_void_linkedlist(struct linkedlist* linked_list, struct node* node){
  linked_list->first = node; //nodo della lista
  linked_list->last = linked_list->first; //diventa il primo e l'ultimo nodo della lista
  linked_list->lenght = 1; //fissa la lunghezza  1
}

struct linkedlist* new_linkedlist(struct node* node){
  struct linkedlist* linked_list = (struct linkedlist*)malloc(sizeof(struct linkedlist)); //alloca nell'heap la linkedlist
  if(node != NULL) //if a value is passed so linked list must have 1 node
      initialize_void_linkedlist(linked_list, node);//initialized with value else a void list is required

  return linked_list; //ritorna il puntatore alla lista linkata
}

int append_node(struct linkedlist* linked_list, struct node* node){ //appende il nodo in fondo alla lista
  if(linked_list == NULL || node == NULL) return 1; //ritorna 1 su errore

  if(linked_list->lenght==0)//se listalinkata è vuota allora non è ancora stata inizializzata
    initialize_void_linkedlist(linked_list, node); //inizializza la lista
  else
  {
    append_last(linked_list->last, node); //no bad error could occur; appende all'ultima posizione il nodo
    linked_list-> last = node; //l'ultimo nodo è il nuovo nodo
    linked_list->lenght++;// incrementa la size
  }
  return 0; //ritorna 0 su successo
}

int append_node_first(struct linkedlist* linked_list, struct node* node){//posiziona il nodo all'inizio della lista
  if(linked_list == NULL || node == NULL) return 1;//ritorna 1 su errore

  if(linked_list->lenght==0)//void linkedlist
    initialize_void_linkedlist(linked_list, node);//inizializza la lista linkata
  else
  {
    insert_first(linked_list->first, node); //no bad error could occur; inserisce l'elemento all'inizio della lista
    linked_list-> first = node; //il nodo aggiunto è il primo della lista
    linked_list->lenght++;// incrementa la size
  }
  return 0; //ritorna 0 su successo
}

int remove_first_from_linked_list(struct linkedlist* linked_list){ //rimuove il primo elemento che si trova nella lista
  struct node* first = linked_list->first; //estrare il primo nodo
  linked_list->first = first->next; //posiziona alla prima posizione il nodo successivo
  if(linked_list->first) //se c'è il nodo
    linked_list->first->prev = NULL; //allora metti il suo precedente a NULL (si trova ora all'inizio della lista)
  //free(first->value);
  free(first); //libera il nodo
  linked_list->lenght--;
  return 0;
}

int remove_node_from_linkedlist(struct node* node, struct linkedlist* linked_list){
  if(node == linked_list->first || equals(node, linked_list->first))//IDK why but if the first element is deleted it make a segmentation fault
  {
    remove_first_from_linked_list(linked_list); //se il nodo è il primo della lista allora rimuovi il primo elemento tramite la funzione specifica
    return 1;
  }
  else if(remove_node(linked_list->first, node)) //altrimenti rimuovi il nodo tramite la funzione generale
  {
    linked_list->lenght--; //decrementa il valore
    return 1;
  }
  return 0;
}

//void clear

// void print_linkedlist(struct linkedlist* linkedlist){
//     struct node* actual_node = linkedlist->first;
//     while(actual_node)
//     {
//       printf("%s\n", ((sender_msg*)actual_node->value)->message);
//       actual_node = actual_node->next;
//     }
// }

// int main(int argc, char const *argv[]) {
//   struct node* node = new_node("HELLO");
//   struct node* node1 = new_node("WOrld");
//   struct node* node2 = new_node("Tired og this shit");
//   struct node* node3 = new_node("3");
//   struct node* node4 = new_node("4");
//
//   struct linkedlist* linked_list = new_linkedlist(node);
//   append_node(linked_list, node1);
//   append_node(linked_list, node2);
//   append_node(linked_list, node3);
//   append_node(linked_list, node4);
//
//
//   // struct node* first = insert_first(node4, node3);
//   // first = insert_first(first, node2);
//   // first = insert_first(first, node1);
//   struct node* actual_node = linked_list->first;
//   while(actual_node)
//   {
//     printf("%s\n", actual_node->value);
//     actual_node = actual_node->next;
//   }
//   printf("\n");
//   remove_node_from_linkedlist(node, linked_list);
//   remove_node_from_linkedlist(node4, linked_list);
//   remove_node_from_linkedlist(node2, linked_list);
//
//
//   actual_node = linked_list->first;
//   while(actual_node)
//   {
//     printf("%s\n", actual_node->value);
//     actual_node = actual_node->next;
//   }
//   printf("lenght: %d\n", linked_list->lenght);
//
//
//
//   return 0;
// }
