#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//TODO if int 4byte or 8byte
//using larger buffer incase int is 4 byte
#define BUFFERSIZE 1024
#define and &&

void hackyBroadcast(const void* buf, int length,int target);
void broadCastGrained();
void broadToNeighbor(const void* buf, int length);
void* announceToNeighbors(void* params);
void checkAlive();
void listenForNeighbors();

void my_sleep(int time);