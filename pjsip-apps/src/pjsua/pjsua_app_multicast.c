#include "pjsua_app_common.h"
#include "multicast_listener.h"
#include "multicast_sender.h"
#include "event_handler.h"

#include <stdbool.h>

pj_status_t multicast_main()
{
    char* buf[65535];
    bool exit_1 = false;
    while (!exit_1) {
       int rc = multicast_listener_read(buf,sizeof(buf));
       if(rc > 0) {
           printf("READ FROM MULTICAST: %s\n",buf);
           event_handler_run(buf);
       } else {
           printf("READ FROM MULTICAST: ERROR\n");
       }
    }

    return PJ_SUCCESS;
}
