#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

struct node {
  struct node* next;
  struct node* prev;
  void* value;
};

struct linkedlist {
  struct node* first;
  struct node* last;
  int lenght;

  pthread_mutex_t mutex;
};

//NODE
//UTILITY FUNCTIONS
int equals(struct node* node, struct node* node1){
  return node == node1 || node->value == node1->value; //NON SONO SICURO FUNZIONI
}

struct node* insert_first(struct node* first, struct node* insert){
  insert->next = first;
  insert-> prev = NULL;
  first->prev = insert;
  return insert;
}

int append_last(struct node* last, struct node* insert){
  if(last == NULL || insert == NULL) return 1;
  insert->prev = last;
  insert-> next = NULL;
  last->next = insert;
  return 0;
}

int remove_node(struct node* first, struct node* node_to_remove){
  while(first)//while node != NULL, aren't at the end
  {
    if(equals(first, node_to_remove)) //check nodes
      {
        //save prev and next
        struct node* prev = first->prev;
        struct node* next = first->next;
        //free(first->value);//could be allocated in heap
        free(first);//deallocate space
        //link prev and next
        if(prev!=NULL) prev->next = next;
        if(next!=NULL) next->prev = prev;
        return 1;
      }
      first = first->next; //go for next node
    }
  return 0;
}


//INSTANTIATION FUNCTIONS
struct node* new_node(void* value){
  struct node *node = (struct node*)malloc(sizeof(struct node));
  (*node).next = NULL;
  (*node).prev = NULL;
  (*node).value = value;
  return node;
}

struct node* new_node_params(struct node* prev, struct node* next, void* value){
  //initialize node with next and prev nodes
  struct node *node = (struct node*)malloc(sizeof(struct node));
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
  linked_list->first = node;
  linked_list->last = linked_list->first;
  linked_list->lenght = 1;
}

struct linkedlist* new_linkedlist(struct node* node){
  struct linkedlist* linked_list = (struct linkedlist*)malloc(sizeof(struct linkedlist));
  if(node != NULL) //if a value is passed so linked list must have 1 node
      initialize_void_linkedlist(linked_list, node);//initialized with value else a void list is required

  return linked_list;
}

int append_node(struct linkedlist* linked_list, struct node* node){
  if(linked_list == NULL || node == NULL) return 1;

  if(linked_list->lenght==0)//void linkedlist
    initialize_void_linkedlist(linked_list, node);
  else
  {
    append_last(linked_list->last, node); //no error could occur
    linked_list-> last = node;
    linked_list->lenght++;// = linked_list-> ++lenght;
  }
  return 0;
}

int remove_node_from_linkedlist(struct node* node, struct linkedlist* linked_list){
  if(node == linked_list->first)//IDK why but if the first element is deleted it make a segmentation fault
    {
      linked_list->first = node->next;
      linked_list->lenght--;
      free(node);
    }
  else if(remove_node(linked_list->first, node))
    linked_list->lenght--;
  return 0;
}

void print_linkedlist(struct linkedlist* linkedlist){
    struct node* actual_node = linkedlist->first;
    while(actual_node)
    {
      printf("%d\n", *(int*)actual_node->value);
      actual_node = actual_node->next;
    }
}

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
