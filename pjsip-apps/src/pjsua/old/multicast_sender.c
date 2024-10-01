#include "multicast_sender.h"
#include "multicast_const.h"


static int fd = -1;
static struct sockaddr_in addr;


void multicast_sender_close() {
    if(fd >= 0) {
        close(fd);
    }
    fd=-1;
}

int multicast_sender_init()
{
    const char* group = MULTICAST_GROUP;
    int port = MULTICAST_PORT_LISTEN_ON_EXTERNAL_APP;

    // create what looks like an ordinary UDP socket
    //
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        multicast_sender_close();
        return -1;
    }

    // set up destination address
    //
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(group);
    addr.sin_port = htons(port);
    return 0;
}


int multicast_sender_write(char* msgbuf,size_t msgbuf_len) {
    if(fd < 0 ) {
       if(multicast_sender_init() < 0) {
           fd= -1;
           return -1;
       }
    }
    char ch = 0;
    int nbytes = sendto(
        fd,
        msgbuf,
        msgbuf_len,
        0,
        (struct sockaddr*) &addr,
        sizeof(addr)
        );
    if (nbytes < 0) {
        perror("sendto");
        multicast_sender_close();
        return -2;
    }
    return nbytes;
}
