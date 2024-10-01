#ifndef MULTICAST_SENDER_H_
#define MULTICAST_SENDER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for sleep()

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int multicast_sender_write(char* msgbuf,size_t msgbuf_len);

#endif // MULTICAST_SENDER_H_
