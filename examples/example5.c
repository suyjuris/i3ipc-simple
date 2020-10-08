/* examples/example5.c */
#define I3IPC_IMPLEMENTATION
#include "../i3ipc.h"
    
int main(int argc, char** argv) {
    i3ipc_run_command_simple("invalid_command");
}
