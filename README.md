
# i3ipc-simple

This is a C library to interact with i3's [IPC interface](https://i3wm.org/docs/ipc.html) for C and C++ applications. It is intended to be easy to use. Example program:

```C
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
```

Compile and run this with `gcc examples/example1.c -o example1 && ./example1` .

## Features

* Single-header library
* No dependencies (apart from OS and stdlib)
* Compatible with C99 (or newer) and C++98 (or newer)
* Initialisation and error handling are optional
* It's Just A C API

As it has (to the extent of my knowledge) not been used by anyone apart from myself, **this library should not be considered particularly robust**. You may very well run into bugs. However, I have done some amount of testing (including fuzz-testing).

Also keep in mind that I run a Linux machine, so **you may run into problems on non-Linux operating systems**, as I cannot test on them.

# Overview

While the actual API is documented in the source code, I want to give a brief overview of how to use this library.

## Installation

Copy `i3ipc.h` into your project. This is a single-header library, so include it as usual and `#define I3IPC_IMPLEMENTATION` exactly once before the `#include` . If you have only a single file, it would look like this:

```C
#include ...
#include ...

#define I3IPC_IMPLEMENTATION
#include "i3ipc.h"
```

There are no dependencies you have to install. (`i3ipc-simple` does depend on the C standard library and OS headers.) You do not need to specify any special compiler flags and the code should compile without warnings. (If you do get warnings, please file an issue.)

At this point you can jump directly into making API calls. No initialisation is necessary. See `i3ipc.h` for the available functions.

## Receiving events

Basically, you subscribe to events of the types you are interested in using `i3ipc_subscribe` or `i3ipc_subscribe_single` and wait for an event to arrive with `i3ipc_event_next` . For example:

```C
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
```

Compile and run this with `gcc examples/example2.c -o example2 && ./example2` .

If you want to receive multiple types of events, it is preferable to call `i3ipc_subscribe` once instead of multiple calls to `i3ipc_subscribe_single`, like this:

```C
int types[] = {I3IPC_EVENT_WINDOW, I3IPC_EVENT_WORKSPACE};
i3ipc_subscribe(types, sizeof(types) / sizeof(types[0]));
```

Either one should work, but having a single call to `i3ipc_subscribe` may avoid memory allocation and a more complicated codepath.

### Events from multiple sources

The `i3ipc_event_next` interface breaks down if your application also needs to work with non-i3 events (e.g. X11 events). To solve this, `i3ipc_event_fd` provides you with the file descriptor of the socket used for events. You can use it like this:

```C
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
```

Here, `poll` waits until either the socket used to receive events from i3 or standard input becomes readable. Then we check for events from i3 with a timeout of 0. If there are no events, we get `NULL` and nothing is done. We can also use the return value from `poll` to determine which file descriptor became readable.

## Memory management

By default, you need to `free` the pointers returned to you. This may be tedious and error-prone, so you can call `i3ipc_set_staticalloc(true)` to change this behaviour: Memory will be taken from a global buffer, which is reused between calls. If you do this, the data returned to you is valid only until the next call to any `i3ipc-simple` function. (Technically, only those which use memory from this region, but you cannot tell without reading the source code.) Example:

```C
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
```

Another important topic is the amount of allocations done by `i3ipc-simple`. Mostly, it uses persistent, growing buffers. After some amount of time these will be large enough and no further allocations are performed.

(There is one exception. If messages arrive out-of-order, which can happen only on the event socket and only in case of multiple `i3ipc_subscribe` calls, they will be stored on the heap. This should not be a problem.)

## Error handling

The default error handling strategy is to panic, i.e. abort the program with a (hopefully informative) error message, which looks like this:

```C
/* examples/example5.c */
#define I3IPC_IMPLEMENTATION
#include "../i3ipc.h"

int main(int argc, char** argv) {
    i3ipc_run_command_simple("invalid_command");
}
```

Trying to run this will give the following.

```
$ gcc examples/example5.c -o example5 && ./example5
Error: run command failed
Error: with error: 'Expected one of these tokens: <end>, '[', 'move', 'exec', 'exit', 'restart',
'reload', 'shmlog', 'debuglog', 'border', 'layout', 'append_layout', 'workspace', 'focus',
'kill', 'open', 'fullscreen', 'sticky', 'split', 'floating', 'mark', 'unmark', 'resize',
'rename', 'nop', 'scratchpad', 'swap', 'title_format', 'mode', 'bar''
Error: while executing command: 'invalid_command'
Aborted (core dumped)
$
```

That means that you can write your application without caring about handling error states, as your program will never survive to live with their consequences. Of course, for some applications this behaviour is undesirable. Instead, call `i3ipc_set_nopanic(true)` to make errors non-fatal and use `i3ipc_error_code` to check for their presence:

```C
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
```

After an error occurred, the library enters an error state and most functions simply do nothing (returning NULL or equivalent). This is fine, and you might use this behaviour to execute a sequence of operations and check for failure only at the end.

To go back to normal operations, call `i3ipc_error_reinitialize` . If communication with i3 went wrong (e.g. an IO failure or i3 returned malformed data), the connection is closed, and `i3ipc_error_reinitialize` attempts to reconnect. It is also possible that an error is reported within i3's IPC protocol (e.g. a command that failed to execute, as in the example above). There it does not make sense to re-open the connection, so `i3ipc_error_reinitialize` simply resets the error state in that case.

## Data structures

The data structures are modelled closely after the messages received from i3, so you should consult its [documentation](https://i3wm.org/docs/ipc.html) for information about the precise meaning of individual attributes.

* The name of each attribute corresponds exactly to its name in the json representation, with one exception: `class` is named `window_class` instead, for compatibility with C++ code.
* Strings are represented by a zero-terminated `char*` and a length. For ordinary attributes these are split into two members, `<name>` and `<name>_size` . Many strings have only a limited set of possible values, for these an additional `<name>_enum` member exists, which you can compare with symbolic constants, as in `example2.c` above:

    ```C
    I3ipc_event_window* ev = (I3ipc_event_window*)ev_any;
    if (ev->change_enum == I3IPC_WINDOW_CHANGE_FOCUS) {
        printf("focused window: %s\n", ev->container.name);
    }
    ```

* An array of type `T` is represented by a pointer and size, with `<name>` and `<name>_size` members, respectively. Arrays of strings need special treatment, there the strings use `I3ipc_string`. (Internally, strings are just arrays of `char` and any array of arrays would need to be handled similarly.)
* Some fields may be `null` in the JSON representation, which causes zero-initialisation. Hence strings, arrays and pointers will be NULL in that case. For primitive types, there is no way to differentiate between the values `0` and `NULL`, so an additional `<name>_set` member is provided, which indicates whether the attribute was set.

## Debugging

Call `i3ipc_set_loglevel(1)` to turn on debug messages. This will dump all messages that are exchanged between `i3ipc-simple` and i3.

You can serialise most data structures to JSON, via `i3ipc_printjson`. Example:

```C
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
```

Its not very beautiful, so you may want to pass the output to a JSON pretty-printer.

## Miscellaneous

* There is a low-level API, which allows you to manually send messages to i3 among other things.
* The library initialises automatically when you call the first function. If you want more control, you can use `i3ipc_init_try` .
* You can define a few macros to influence how some features are implemented. Currently there are `I3IPC_ALIGNOF(T)` which should return the alignment of type `T`, and `I3IPC_ANONYMOUS_UNION`, which is either 0 or 1, indicating whether the build support anonymous unions. Both should be initialised to reasonable defaults.

# Issues, contributions and feedback

You are welcome to file issues or pull requests! Alternatively, you can send me an [e-mail](mailto:i3ipc-simple@nicze.de). If you have used this library (successfully or not), I would love to hear about your experience, so please do not hesitate to leave a bit of feedback.

## Tests

To run the tests, you need to compile and run `i3ipc_test.c` :

    $ ./test/build.sh base && ./build/i3ipc_test evaluate test/tests

If all of them pass, you are good to go. Else, you can investigate the specific test that failed by running

    $ ./build/i3ipc_test execute <path_to_test>

Additional options for testing are described briefly in the documentation of `test/build.sh`.
