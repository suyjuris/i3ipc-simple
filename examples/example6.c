/* examples/example6.c */
#define I3IPC_IMPLEMENTATION
#include "../i3ipc.h"
    
int main(int argc, char** argv) {
    i3ipc_set_nopanic(true);
    i3ipc_run_command_simple("invalid_command");
    if (i3ipc_error_code() == I3IPC_ERROR_FAILED) {
        puts("The command failed, but the program lives on!");
        i3ipc_error_reinitialize(false);
    }
}
