/* examples/example1.c */
#define I3IPC_IMPLEMENTATION
#include "../i3ipc.h"

int main(int argc, char** argv) {
    I3ipc_reply_workspaces* reply = i3ipc_get_workspaces();
    for (int i = 0; i < reply->workspaces_size; ++i) {
        printf("found workspace %s on output %s\n",
            reply->workspaces[i].name, reply->workspaces[i].output);
    }
    free(reply);
}
