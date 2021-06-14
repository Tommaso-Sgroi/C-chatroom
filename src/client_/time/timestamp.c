#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../datastructures/size.h"


/*estrae il timetamp e lo rende human readble*/
void get_timestamp(char* buffer){
  memset(buffer, 0,BUFFER_DATE_SIZE); //azzera il buffer
	time_t t = time(NULL);
	struct tm tm = *localtime(&t); //prende il tempo locale
  snprintf(buffer, BUFFER_DATE_SIZE, "%d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  //inserisce nel buffer la stringa contenente la data timestamp in quel formato, che corrisponde alla data ore minuti e secondi dell'invio del messaggio
}
