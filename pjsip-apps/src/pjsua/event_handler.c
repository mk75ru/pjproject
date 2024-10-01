#include "event_handler.h"
#include "string.h"

void event_handler_run(char* event) {
    multicast_sock_write(event,strlen(event)+1);
}

