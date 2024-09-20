#include "multicast_listener.h"
#include "multicast_const.h"

static int fd = -1;
struct sockaddr_in addr;
#define MSGBUFSIZE 65535

void multicast_listener_close() {
    if(fd >= 0) {
        close(fd);
    }
    fd=-1;
}

//char* msgbuf
int multicast_listener_init()
{
    const char* group = MULTICAST_GROUP;
    int port = MULTICAST_PORT_LISTEN_ON_PJSUA_APP;
    const char* ip_iface = MULTICAST_IP_IFACE;

    // create what looks like an ordinary UDP socket
    //
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        multicast_listener_close();
        return -1;
    }

    // allow multiple sockets to use the same PORT number
    //
    u_int yes = 1;
    if (
        setsockopt(
            fd, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)
        ) < 0
    ){
       perror("Reusing ADDR failed");
       multicast_listener_close();
       return -2;
    }

        // set up destination address
    //
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    addr.sin_port = htons(port);


    // bind to receive address
    //
    if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind");
        multicast_listener_close();
        return -3;
    }

    // use setsockopt() to request that the kernel join a multicast group
    //
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(group);
    mreq.imr_interface.s_addr = inet_addr(ip_iface);//htonl(INADDR_ANY);
    if (
        setsockopt(
            fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)
        ) < 0
    ){
        perror("setsockopt");
        multicast_listener_close();
        return -4;
    }

    char loopch = 1;
    if(setsockopt(
           fd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0) {
        perror("Setting IP_MULTICAST_LOOP error");
        multicast_listener_close();
        return -5;
    }

    return 0;
}

//char msgbuf[MSGBUFSIZE];
char*  multicast_listener_malloc() {
   return  malloc(MSGBUFSIZE);
}
void  multicast_listener_free(char* msgbuf) {
     free(msgbuf);
}

int multicast_listener_read(char* msgbuf,size_t msgbuf_len) {
    if(fd < 0 ) {
       if(multicast_listener_init() < 0) {
           fd= -1;
           return -1;
       }
    }

    memset(msgbuf,0,msgbuf_len);
    int addrlen = sizeof(addr);
    int nbytes = recvfrom(
        fd,
        msgbuf,
        msgbuf_len,
        0,
        (struct sockaddr *) &addr,
        &addrlen
        );
    if (nbytes <= 0) {
        perror("recvfrom");
        multicast_listener_close();
        return -2;
    }
    msgbuf[nbytes] = '\0';
    puts(msgbuf);
    return nbytes;
}

