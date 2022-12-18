/* examples/swapcycle.c */

#include <assert.h>

#define I3IPC_IMPLEMENTATION
#include "i3ipc.h"

void ensure(char** buf, int* size, int capacity) {
    if (*size < capacity) {
        *size *= 2;
        if (*size < capacity) *size = capacity;
        *buf = realloc(*buf, *size);
        assert(*buf);
    }
}

char* global_buf;
int global_buf_size;
#define buf_printf(fmt, ...) ( \
        ensure(&global_buf, &global_buf_size, snprintf(NULL, 0, fmt, __VA_ARGS__) + 1), \
        snprintf(global_buf, global_buf_size, fmt, __VA_ARGS__), \
        global_buf)
    

int main() {
    I3ipc_reply_workspaces* reply = i3ipc_get_workspaces();

    // Filter so that only visible workspaces remain
    {int i_out = 0;
    for (int i = 0; i < reply->workspaces_size; ++i) {
        if (!reply->workspaces[i].visible) continue;
        reply->workspaces[i_out++] = reply->workspaces[i];
    }
    reply->workspaces_size = i_out;}
    
    // Sort by x-coordinate (insertion sort)
    for (int i = 1; i < reply->workspaces_size; ++i) {
        for (int j = i-1; j >= 0; --j) {
            if (reply->workspaces[i].rect.x >= reply->workspaces[j].rect.x) break;
            I3ipc_reply_workspaces_el tmp = reply->workspaces[i];
            reply->workspaces[i] = reply->workspaces[j];
            reply->workspaces[j] = tmp;
        }
    }

    // Find focused workspace
    int focused = -1;
    for (int i = 0; i < reply->workspaces_size; ++i) {
        if (!reply->workspaces[i].focused) continue;
        assert(focused == -1);
        focused = i;
    }
    assert(focused != -1);
    
    // Move workspaces
    for (int i = 0; i < reply->workspaces_size; ++i) {
        int j = (i+1) % reply->workspaces_size;
        char* cmd = buf_printf("[con_id=%lld] focus; move workspace to output %s",
            (long long)reply->workspaces[i].id, reply->workspaces[j].output);
        i3ipc_run_command_simple(cmd);
    }
    
    // Focus the workspace that is now where the previously focused workspace was
    {int i = (focused-1 + reply->workspaces_size) % reply->workspaces_size;
    char* cmd = buf_printf("[con_id=%lld] focus", (long long)reply->workspaces[i].id);
    i3ipc_run_command_simple(cmd);}

    free(reply);
}
