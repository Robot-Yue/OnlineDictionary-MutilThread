#ifndef DIC_H
#define DIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sqlite3.h>

#define  R  1   //  user register
#define  L  2   //  user login
#define  Q  3   //  query word
#define  H  4   //  history record 
#define  E  5   //  exit

typedef struct 
{
	int type;	   	   
	char name[20];	   
	char data[1024];   
}MSG_t;
typedef struct
{
	int fd;
	char IP[20];
}ThreadParam_t;

#endif
