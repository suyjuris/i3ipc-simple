/* examples/example3.c */
#define I3IPC_IMPLEMENTATION
#include "../i3ipc.h"
    
int main(int argc, char** argv) {
    puts("Press return to exit...");
    i3ipc_subscribe_single(I3IPC_EVENT_WINDOW);

    struct pollfd polls[2];
    polls[0].fd = i3ipc_event_fd();
    polls[0].events = POLLIN;
    polls[1].fd = STDIN_FILENO;
    polls[1].events = POLLIN;

    while (true) {
        int code = poll(polls, sizeof(polls)/sizeof(polls[0]), -1);
        if (code == -1) { perror("Error"); exit(1); }
        
        I3ipc_event* ev_any = i3ipc_event_next(0);
        if (ev_any && ev_any->type == I3IPC_EVENT_WINDOW) {
            I3ipc_event_window* ev = (I3ipc_event_window*)ev_any;
            if (ev->change_enum == I3IPC_WINDOW_CHANGE_FOCUS) {
                printf("focused window: %s\n", ev->container.name);
            }
        }
        free(ev_any);

        if (polls[1].revents & POLLIN) break;
    }
}
