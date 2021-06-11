#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../datastructure/size.h"

void get_timestamp(char* buffer){
  memset(buffer, 0,BUFFER_DATE_SIZE);
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
  snprintf(buffer, BUFFER_DATE_SIZE, "%d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}
