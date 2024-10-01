#ifndef multicast_sock_H_
#define multicast_sock_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MULTICAST_GROUP     "225.0.0.37"
#define MULTICAST_PORT_IPC  3421
#define MULTICAST_IP_IFACE  "127.0.0.1"

char* multicast_sock_malloc();
void  multicast_sock_free();
int   multicast_sock_read(char* msgbuf,size_t msgbuf_len);
int   multicast_sock_write(char* msgbuf,size_t msgbuf_len) ;

#endif // multicast_sock_H_
