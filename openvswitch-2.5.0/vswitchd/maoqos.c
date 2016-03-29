
#include <stdio.h>

#include "maoqos.h"





//int maoLog(char * logStr);
int maoLog(char * logStr){

	if (NULL == logStr)
		return 2;

	FILE * log = fopen("/home/mao/maoOVSlog/1.txt","a+");
	if(NULL == log)
		return 1;

	fputs(logStr, log);

	fclose(log);

	return 0;
}
