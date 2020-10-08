#!/bin/bash

set -eu

GCC=gcc
GXX=g++
CLANG=clang
CLANGXX=clang++
AFL_GCC=afl-gcc
AFL_FUZZ=afl-fuzz
CFLAGS="-Werror -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fmax-errors=2 -ggdb"
CFLAGS_PEDANTIC="-Werror -Wall -Wextra -Wpedantic -ggdb -O3"
CFLAGS_SANITIZE="$CFLAGS_PEDANTIC -fsanitize=address -fsanitize=undefined"
PRE="AFL_PRELOAD=/usr/local/lib/afl/libdislocator.so"

cd "$(dirname "$0")"

if [ "$#" -lt 1 ]; then
    echo "Usage:"
    echo "  $0 [base|sanitize|sanitize_clang|pedantic|fuzz|fuzz_run]"
    echo
    echo "Modes:"
    echo "  base            Default executable for testing (gcc)"
    echo "  sanitize        Instrument with sanitization for undefined behaviour and memory issues (gcc)"
    echo "  sanitize_clang  Same, but with clang. On my machine, this gives better output. (clang)"
    echo "  pedantic        Compile a bunch of executables with lots of warnings enabled. (gcc, clang)"
    echo "  fuzz            Binary with instrumentation for fuzzing and some hardening (afl-gcc)"
    echo "  fuzz_run        Set up the environment for fuzzing. May only work on my machine."
    echo
    echo "All executables are built into ../build"
    exit 1
fi;

function E {
    echo $*
    $*
}

mkdir -p ../build

if [ "$1" = "base" ]; then
    "$GCC" $CFLAGS -O0 i3ipc_test.c -o ../build/i3ipc_test
elif [ "$1" = "sanitize" ]; then
    "$GCC" $CFLAGS_SANITIZE i3ipc_test.c -static-libasan -o ../build/i3ipc_test_sanitize
elif [ "$1" = "sanitize_clang" ]; then
    "$CLANG" $CFLAGS_SANITIZE i3ipc_test.c -o ../build/i3ipc_test_sanitize_clang
elif [ "$1" = "pedantic" ]; then
    cp i3ipc_test.c ../build/i3ipc_test.cpp
    E "$GCC"     $CFLAGS_PEDANTIC -std=c99   i3ipc_test.c            -o ../build/i3ipc_test_c99
    E "$GCC"     $CFLAGS_PEDANTIC -std=c11   i3ipc_test.c            -o ../build/i3ipc_test_c11
    E "$GCC"     $CFLAGS_PEDANTIC -std=c17   i3ipc_test.c            -o ../build/i3ipc_test_c17
    E "$CLANG"   $CFLAGS_PEDANTIC -std=c99   i3ipc_test.c            -o ../build/i3ipc_test_c99_clang
    E "$CLANG"   $CFLAGS_PEDANTIC -std=c11   i3ipc_test.c            -o ../build/i3ipc_test_c11_clang
    E "$CLANG"   $CFLAGS_PEDANTIC -std=c17   i3ipc_test.c            -o ../build/i3ipc_test_c17_clang
    E "$GXX"     $CFLAGS_PEDANTIC -std=c++98 ../build/i3ipc_test.cpp -o ../build/i3ipc_test_c++98
    E "$GXX"     $CFLAGS_PEDANTIC -std=c++03 ../build/i3ipc_test.cpp -o ../build/i3ipc_test_c++03
    E "$GXX"     $CFLAGS_PEDANTIC -std=c++11 ../build/i3ipc_test.cpp -o ../build/i3ipc_test_c++11
    E "$GXX"     $CFLAGS_PEDANTIC -std=c++14 ../build/i3ipc_test.cpp -o ../build/i3ipc_test_c++14
    E "$GXX"     $CFLAGS_PEDANTIC -std=c++17 ../build/i3ipc_test.cpp -o ../build/i3ipc_test_c++17
    E "$CLANGXX" $CFLAGS_PEDANTIC -std=c++98 ../build/i3ipc_test.cpp -o ../build/i3ipc_test_c++98_clang
    E "$CLANGXX" $CFLAGS_PEDANTIC -std=c++17 ../build/i3ipc_test.cpp -o ../build/i3ipc_test_c++17_clang
elif [ "$1" = "fuzz" ]; then
    AFL_HARDEN=1 "$AFL_GCC" $CFLAGS -O3 -fstack-protector-all i3ipc_test.c -o ../build/i3ipc_test_fuzz
elif [ "$1" = "fuzz_run" ]; then
    sudo -- sh -c 'echo core >/proc/sys/kernel/core_pattern'
    sudo -- sh -c 'cd /sys/devices/system/cpu; echo performance | tee cpu*/cpufreq/scaling_governor' >/dev/null
    mkdir -p ../build/fuzz/input ../build/fuzz/output
    cp tests/handwritten/execute ../build/fuzz/input
    echo "# Fuzzing environment set up, run the following command (or something similar) to start fuzzing"
    echo "$PRE" "$AFL_FUZZ" -i ../build/fuzz/input -o ../build/fuzz/output -- ../build/i3ipc_test_fuzz fuzz
else
    echo "Error: first argument not recognised"
    exit 1
fi;
