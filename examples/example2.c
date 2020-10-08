/* examples/example2.c */
#define I3IPC_IMPLEMENTATION
#include "../i3ipc.h"
    
int main(int argc, char** argv) {
    i3ipc_subscribe_single(I3IPC_EVENT_WINDOW);
    while (true) {
        I3ipc_event* ev_any = i3ipc_event_next(-1);
        if (ev_any->type == I3IPC_EVENT_WINDOW) {
            I3ipc_event_window* ev = (I3ipc_event_window*)ev_any;
            if (ev->change_enum == I3IPC_WINDOW_CHANGE_FOCUS) {
                printf("focused window: %s\n", ev->container.name);
            }
        }
        free(ev_any);
    }
}
