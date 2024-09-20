#include "event_handler.h"
#include "string.h"
#include "multicast_sender.h"


void event_handler_run(char* event) {
    multicast_sender_write(event,strlen(event)+1);
}

/*
void event_handler_run(char* event) {
    const char* cmd = "/root/event_handler.sh";
    void* cmdfull =  malloc(strlen(cmd) + strlen(" ") + strlen(event) + 1);
    sprintf (cmdfull, "%s %s",cmd,event);
    system(cmdfull);
    free(cmdfull)
}
*/
