
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "datastructure/linkedlist.c"
#include "server.h"


struct tm* get_time(){
	time_t t = time(NULL);
	return localtime(&t);
}

struct node* check_youngest_msg(struct node* node, struct node* other){
  if(node == NULL || other == NULL || node == other) return NULL;
  sender_msg *sender = (sender_msg*) node->value;
  sender_msg *reciver = (sender_msg*) other->value;
  struct node* append_before;
  struct node* actual_node = other;

	timestamp sender_ts = *new_timestamp(sender->message);
  while(actual_node)
  {
		timestamp reciver_ts = *new_timestamp(reciver->message);
    if(sender_ts.year < reciver_ts.year)
			append_before = actual_node;
    else if(sender_ts.year == reciver_ts.year && sender_ts.month < reciver_ts.month)
			append_before = actual_node;
    else if(sender_ts.month == reciver_ts.month && sender_ts.day < reciver_ts.day)
			append_before = actual_node;
    else if(sender_ts.day == reciver_ts.day && sender_ts.hours < reciver_ts.hours)
			append_before = actual_node;
    else if(sender_ts.hours == reciver_ts.hours && sender_ts.min < reciver_ts.min)
			append_before = actual_node;
    else if(sender_ts.min == reciver_ts.min && sender_ts.sec < reciver_ts.sec) //if sender < reciver
			append_before = actual_node;
    actual_node = actual_node->next;
  }
  return append_before;
}
int main(){

	char dateString1[] = "4000-12-10 10:04:54 ajbdkasbdfkjnbfkjsdfkjndkjfn mkmfsm lkfmalkdfm askfm kma";
	char dateString2[] = "2020-01-01 00:00:00 mkmfsm lkfmalkdfm askfm kma";

	//printf("%s\n", buffer);
	sender_msg* msg1 = new_sender_msg(dateString1, -1, "addr1");
	sender_msg* msg2 = new_sender_msg(dateString2, -1, "addr2");

	struct linkedlist* linked = new_linkedlist(NULL);

	struct node* node1 = new_node((void*)msg1);//da inserire
	struct node* node2 = new_node((void*)msg2);

	append_node(linked, node2);

	struct node* append_before = check_youngest_msg(node1, node2);
	printf("Append before of %s\n", append_before == NULL? "NULL": ((sender_msg*)append_before->value)->message);
	struct node* tmp = insert_first(append_before, node1);
	if(linked->lenght == 1 && tmp)
	{
		linked->first = tmp;
	}
	else if(tmp == NULL)
	{
		append_node(linked, node1);
	}

	// char path_prefix [] = "./logs/";
	// char path_suffix[] = ".txt";
	// char path [strlen(path_prefix) + BUFFER_NAME_SIZE + strlen(path_suffix)];
	// char real_name[BUFFER_NAME_SIZE];
	// strncpy(real_name, name, strlen(name)-1);
	// strcat(path, path_prefix);
	// strcat(path, real_name);
	// strcat(path, path_suffix);
	// printf("%s\n", path);


	printf("Message Appended %s\n", tmp == NULL? "NULL": ((sender_msg*)tmp->value)->message);
	struct node* actual_node = linked->first;
	while(actual_node)
	{
		printf("%s\n", ((sender_msg*)actual_node->value)->message);
		actual_node = actual_node->next;
	}
	return 0;

}
