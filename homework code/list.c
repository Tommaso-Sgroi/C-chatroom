#include <stdlib.h>
#include <stdio.h>

struct node {
  struct node* next;
  struct node* prev;
  void* value;
};


//UTILITY FUNCTIONS
int equals(struct node* node, struct node* node1){
  return node->value == node1->value; //NON SONO SICURO FUNZIONI
}

struct node* insert_first(struct node* first, struct node* insert){
  insert->next = first;
  insert-> prev = NULL;
  first->prev = insert;
  return insert;
}

int remove_node(struct node* first, struct node* node_to_remove){
  while(first)//while node != NULL, aren't at the end
  {
    if(equals(first, node_to_remove)) //check nodes
      {
        //save prev and next
        struct node* prev = first->prev;
        struct node* next = first->next;
        free(first);//deallocate space
        //link prev and next
        prev->next = next;
        next->prev = prev;
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



int main(int argc, char const *argv[]) {
  struct node* node = new_node((void*)0);
  struct node* node1 = new_node((void*)1);
  struct node* node2 = new_node((void*)2);
  struct node* node3 = new_node((void*)3);
  struct node* node4 = new_node((void*)4);

  struct node* first = insert_first(node4, node3);
  first = insert_first(first, node2);
  first = insert_first(first, node1);
  struct node* actual_node = insert_first(first, node);

  struct node* n = new_node_params(node2, node3, (void*)69);
  remove_node(actual_node, n);

  while(actual_node)
  {
    printf("%d\n", actual_node->value);
    actual_node = actual_node->next;
  }



  return 0;
}
