/* examples/example4.c */
#define I3IPC_IMPLEMENTATION
#include "../i3ipc.h"
    
int main(int argc, char** argv) {
    i3ipc_set_staticalloc(true);

    {
        I3ipc_reply_workspaces* reply = i3ipc_get_workspaces();
        printf("There are %d workspaces\n", reply->workspaces_size);
    }
    {
        I3ipc_reply_outputs* reply = i3ipc_get_outputs();
        printf("There are %d outputs\n", reply->outputs_size);
    }
    {
        I3ipc_reply_marks* reply = i3ipc_get_marks();
        printf("There are %d marks\n", reply->marks_size);   
    }
}
