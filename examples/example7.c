/* examples/example7.c */
#define I3IPC_IMPLEMENTATION
#include "../i3ipc.h"
    
int main(int argc, char** argv) {
    i3ipc_subscribe_single(I3IPC_EVENT_WINDOW);
    while (true) {
        I3ipc_event* ev_any = i3ipc_event_next(-1);
        i3ipc_printjson(I3IPC_TYPE_EVENT, ev_any, NULL);
        puts("");
        free(ev_any);
    }
}
