#ifndef MULTICAST_LISTENER_H_
#define MULTICAST_LISTENER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char* multicast_listener_malloc();
void  multicast_listener_free();
int   multicast_listener_read(char* msgbuf,size_t msgbuf_len);
int multicast_listener_write(char* msgbuf,size_t msgbuf_len) ;
#endif // MULTICAST_LISTENER_H_
