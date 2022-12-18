#!/bin/bash

set -eu

GCC=gcc
# no -Werror for forwards-compatibility
CFLAGS="-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fmax-errors=2 -ggdb -O0 -I.."

cd "$(dirname "$0")"

if [ "$#" -lt 1 ]; then
    echo "Usage:"
    echo "  $0 [examples|swapcycle|alttab]"
    echo
    echo "Modes:"
    echo "  examples        Example snippets from the documentation"
    echo "  swapcycle       Command that moves visible workspaces in a cycle"
    echo "  alttab          Program to implement a simple alt-tab behaviour, uses xcb and xcb-keysyms"
    echo
    echo "All executables are built into ../build"
    exit 1
fi;

mkdir -p ../build

if [ "$1" = "swapcycle" ]; then
    "$GCC" $CFLAGS swapcycle.c -o swapcycle
elif [ "$1" = "alttab" ]; then
    "$GCC" $CFLAGS alttab.c -lxcb -lxcb-keysyms -o alttab
elif [ "$1" = "examples" ]; then    
    "$GCC" $CFLAGS example1.c -o ../build/example1
    "$GCC" $CFLAGS example2.c -o ../build/example2
    "$GCC" $CFLAGS example3.c -o ../build/example3
    "$GCC" $CFLAGS example4.c -o ../build/example4
    "$GCC" $CFLAGS example5.c -o ../build/example5
    "$GCC" $CFLAGS example6.c -o ../build/example6
    "$GCC" $CFLAGS example7.c -o ../build/example7
else
    echo "Error: first argument not recognised"
    exit 1
fi;
