/* SPDX-License-Identifier:  CC0-1.0 */

/* i3ipc-simple, a simple C/C++ library to interact with i3's IPC interface
 * Written by Philipp Czerner in 2020 <i3ipc-simple@nicze.de>
 * 
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 * 
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 *     http://creativecommons.org/publicdomain/zero/1.0/    */


/* *** How to include ***
 * This is a single-header library, so you need to #define I3IPC_IMPLEMENTATION
 * before including this file to create the implementation. This needs to happen
 * exactly once. For example:
 * 
 *     // file1.c
 *     #include "i3ipc.h"
 *     ... use the functions ...
 *    
 *     // file2.c
 *     #define I3IPC_IMPLEMENTATION
 *     #include "i3ipc.h"
 *     ... use the functions ...
 *
 * The library can be found at https://github.com/suyjuris/i3ipc-simple .
 * The README contains high-level documentation. */


#ifndef I3IPC_INCLUDE_I3IPC_H
#define I3IPC_INCLUDE_I3IPC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Forward declare structs, definitions are below */
typedef struct I3ipc_reply_command          I3ipc_reply_command;
typedef struct I3ipc_reply_workspaces       I3ipc_reply_workspaces;
typedef struct I3ipc_reply_subscribe        I3ipc_reply_subscribe;
typedef struct I3ipc_reply_outputs          I3ipc_reply_outputs;
typedef struct I3ipc_reply_tree             I3ipc_reply_tree;
typedef struct I3ipc_reply_marks            I3ipc_reply_marks;
typedef struct I3ipc_reply_bar_config_ids   I3ipc_reply_bar_config_ids;
typedef struct I3ipc_reply_bar_config       I3ipc_reply_bar_config;
typedef struct I3ipc_reply_version          I3ipc_reply_version;
typedef struct I3ipc_reply_binding_modes    I3ipc_reply_binding_modes;
typedef struct I3ipc_reply_config           I3ipc_reply_config;
typedef struct I3ipc_reply_tick             I3ipc_reply_tick;
typedef struct I3ipc_reply_sync             I3ipc_reply_sync;
typedef union  I3ipc_event                  I3ipc_event;

/* *** Core API *** */

/* Run a command. Failure will cause an error. */
void i3ipc_run_command_simple(char const* command);

/* Run a command and return results.
 * You have to free() the result, unless staticalloc is set. */
I3ipc_reply_command* i3ipc_run_command(char const* commands);

/* Subscribe to events of that type.
 * See I3ipc_event_type for possible values for event_type.
 * If you are interested in multiple types, i3ipc_subscribe is the better option. */
void i3ipc_subscribe_single(int event_type);

/* Subscribe events of the specified types.
 * See I3ipc_event_type for possible values for event_type. */
void i3ipc_subscribe(int* event_type, int event_type_size);

/* Wait for the next event, and return it.
 * If timeout_ms milliseconds elapse before an event arrives, return NULL.
 * Negative timeout_ms causes this to wait forever, zero has it return immediately.
 * You have to free() the result, unless staticalloc is set. */
I3ipc_event* i3ipc_event_next(int timeout_ms);

/* Query only major, minor and patch numbers.
 * out_major, out_minor, out_patch are output parameters, they may be NULL
 * You can query more detailed information using i3ipc_get_version . */
void i3ipc_get_version_simple(int* out_major, int* out_minor, int* out_patch);

/* Query the specified information.
 * You have to free() the result, unless staticalloc is set. */
I3ipc_reply_workspaces*     i3ipc_get_workspaces(void);
I3ipc_reply_outputs*        i3ipc_get_outputs(void);
I3ipc_reply_tree*           i3ipc_get_tree(void);
I3ipc_reply_marks*          i3ipc_get_marks(void);
I3ipc_reply_bar_config_ids* i3ipc_get_bar_config_ids(void);
I3ipc_reply_version*        i3ipc_get_version(void);
I3ipc_reply_binding_modes*  i3ipc_get_binding_modes(void);
I3ipc_reply_config*         i3ipc_get_config(void);

/* Query the bar configuration of that name.
 * A list of names can be queried by i3ipc_get_bar_config_ids.
 * You have to free() the result, unless staticalloc is set. */
I3ipc_reply_bar_config* i3ipc_get_bar_config(char const* name);

/* Send a tick with the specified payload to subscribers of tick events. */
void i3ipc_send_tick(char const* payload);

/* Send a sync message. See the i3 documentation for details. */
void i3ipc_sync(int random_value, size_t window);

/* Return the file descriptor for the socket used for events.
 * You can use this if you want to wait on multiple sources, e.g. with poll(). */
int i3ipc_event_fd(void);

/* Same as i3ipc_event_fd, but for messages.
 * This is not as useful. */
int i3ipc_message_fd(void);

/* Set the staticalloc flag, return the old value.
 * If this flag is set, you do not have to free results of the functions above,
 * but only the last one is valid. See the README for details.*/
bool i3ipc_set_staticalloc(bool value);

/* Set the loglevel, return the old value.
 * This controls how much information is printed to the terminal.
 * Values are -1 (silent), 0 (errors, default), 1 (debug messages) */
int i3ipc_set_loglevel(int value);

/* *** Data structures. ***
 * See the README for details.
 * Uninitialised members are NULL (for arrays, strings and pointers) or have a
 *  <name>_set member indicating whether they are set.
 * Arrays and strings have a <name>_size member indicating their length.
 * Strings which can take only a limited set of values have a <name>_enum member,
 *  which you can check using integer comparison. The <type>_value enum lists
 *  possible values. Invalid values are represented as -1.
 * Names of members correspond exactly to their JSON representation, except for
 *  'class', which is named 'window_class' to make C++ people happy. */

typedef struct I3ipc_rect {
    int x;
    int y;
    int width;
    int height;    
} I3ipc_rect;

typedef struct I3ipc_string {
    char* str;
    int str_size;
} I3ipc_string;

enum I3ipc_node_values {
    I3IPC_NODE_TYPE_ROOT = 0,
    I3IPC_NODE_TYPE_OUTPUT,
    I3IPC_NODE_TYPE_CON,
    I3IPC_NODE_TYPE_FLOATING_CON,
    I3IPC_NODE_TYPE_WORKSPACE,
    I3IPC_NODE_TYPE_DOCKAREA,
    
    I3IPC_NODE_BORDER_NORMAL = 0,
    I3IPC_NODE_BORDER_NONE,
    I3IPC_NODE_BORDER_PIXEL,

    I3IPC_NODE_LAYOUT_SPLITH = 0,
    I3IPC_NODE_LAYOUT_SPLITV,
    I3IPC_NODE_LAYOUT_STACKED,
    I3IPC_NODE_LAYOUT_TABBED,
    I3IPC_NODE_LAYOUT_DOCKAREA,
    I3IPC_NODE_LAYOUT_OUTPUT,

    I3IPC_NODE_ORIENTATION_NONE = 0,
    I3IPC_NODE_ORIENTATION_HORIZONTAL,
    I3IPC_NODE_ORIENTATION_VERTICAL,

    I3IPC_NODE_WINDOW_TYPE_NULL = 0,
    I3IPC_NODE_WINDOW_TYPE_NORMAL,
    I3IPC_NODE_WINDOW_TYPE_DIALOG,
    I3IPC_NODE_WINDOW_TYPE_UTILITY,
    I3IPC_NODE_WINDOW_TYPE_TOOLBAR,
    I3IPC_NODE_WINDOW_TYPE_SPLASH,
    I3IPC_NODE_WINDOW_TYPE_MENU,
    I3IPC_NODE_WINDOW_TYPE_DROPDOWN_MENU,
    I3IPC_NODE_WINDOW_TYPE_POPUP_MENU,
    I3IPC_NODE_WINDOW_TYPE_TOOLTIP,
    I3IPC_NODE_WINDOW_TYPE_NOTIFICATION,
    I3IPC_NODE_WINDOW_TYPE_DOCK,
    I3IPC_NODE_WINDOW_TYPE_UNKNOWN
};

typedef struct I3ipc_node_window_properties {
    char* title;
    int   title_size;
    char* instance;
    int   instance_size;
    char* window_class;
    int   window_class_size;
    char* window_role;
    int   window_role_size;
    int   transient_for;
    bool  transient_for_set;
} I3ipc_node_window_properties;

typedef struct I3ipc_node I3ipc_node;
struct I3ipc_node {
    size_t id;
    char*  name;
    int    name_size;
    char*  type;
    int    type_size;
    int    type_enum;
    char*  border;
    int    border_size;
    int    border_enum;
    int    current_border_width;
    char*  layout;
    int    layout_size;
    int    layout_enum;
    char*  orientation;
    int    orientation_size;
    int    orientation_enum;
    float  percent;
    bool   percent_set;
    I3ipc_rect rect;
    I3ipc_rect window_rect;
    I3ipc_rect deco_rect;
    I3ipc_rect geometry;
    int    window;
    bool   window_set;
    I3ipc_node_window_properties* window_properties;
    char* window_type;
    int   window_type_size;
    int   window_type_enum;
    bool  urgent;
    I3ipc_string* marks;
    int           marks_size;
    bool    focused;
    size_t* focus;
    int     focus_size;
    int     fullscreen_mode;
    I3ipc_node* nodes;
    int         nodes_size;
    I3ipc_node* floating_nodes;
    int         floating_nodes_size;
};


enum I3ipc_reply_bar_config_values {
    I3IPC_BAR_CONFIG_MODE_DOCK = 0,
    I3IPC_BAR_CONFIG_MODE_HIDE,
    
    I3IPC_BAR_CONFIG_POSITION_BOTTOM = 0,
    I3IPC_BAR_CONFIG_POSITION_TOP
};

typedef struct I3ipc_bar_config_colors {
    char* background;
    int   background_size;
    char* statusline;
    int   statusline_size;
    char* separator;
    int   separator_size;
    char* focused_background;
    int   focused_background_size;
    char* focused_statusline;
    int   focused_statusline_size;
    char* focused_separator;
    int   focused_separator_size;
    char* focused_workspace_text;
    int   focused_workspace_text_size;
    char* focused_workspace_bg;
    int   focused_workspace_bg_size;
    char* focused_workspace_border;
    int   focused_workspace_border_size;
    char* active_workspace_text;
    int   active_workspace_text_size;
    char* active_workspace_bg;
    int   active_workspace_bg_size;
    char* active_workspace_border;
    int   active_workspace_border_size;
    char* inactive_workspace_text;
    int   inactive_workspace_text_size;
    char* inactive_workspace_bg;
    int   inactive_workspace_bg_size;
    char* inactive_workspace_border;
    int   inactive_workspace_border_size;
    char* urgent_workspace_text;
    int   urgent_workspace_text_size;
    char* urgent_workspace_bg;
    int   urgent_workspace_bg_size;
    char* urgent_workspace_border;
    int   urgent_workspace_border_size;
    char* binding_mode_text;
    int   binding_mode_text_size;
    char* binding_mode_bg;
    int   binding_mode_bg_size;
    char* binding_mode_border;
    int   binding_mode_border_size;         
} I3ipc_bar_config_colors;
    
typedef struct I3ipc_bar_config {
    char* id;
    int   id_size;
    char* mode;
    int   mode_size;
    int   mode_enum;
    char* position;
    int   position_size;
    int   position_enum;
    char* status_command;
    int   status_command_size;
    char* font;
    int   font_size;
    bool  workspace_buttons;
    bool  binding_mode_indicator;
    bool  verbose;
    I3ipc_bar_config_colors colors;
} I3ipc_bar_config;


/* see https://i3wm.org/docs/ipc.html#_command_reply */
typedef struct I3ipc_reply_command_el {
    bool  success;
    char* error;
    int   error_size;
} I3ipc_reply_command_el;

struct I3ipc_reply_command {
    I3ipc_reply_command_el * commands;
    int commands_size;
};

/* see https://i3wm.org/docs/ipc.html#_workspaces_reply */
typedef struct I3ipc_reply_workspaces_el {
    size_t id;
    int    num;
    char*  name;
    int    name_size;
    bool   visible;
    bool   focused;
    bool   urgent;
    I3ipc_rect rect;
    char*  output;
    int    output_size;
} I3ipc_reply_workspaces_el;

struct I3ipc_reply_workspaces {
    I3ipc_reply_workspaces_el* workspaces;
    int workspaces_size;
};

/* see https://i3wm.org/docs/ipc.html#_subscribe_reply */
struct I3ipc_reply_subscribe {
    bool success;
};

/* see https://i3wm.org/docs/ipc.html#_outputs_reply */
typedef struct I3ipc_reply_outputs_el {
        char* name;
        int   name_size;
        bool  active;
        bool  primary;
        char* current_workspace;
        int   current_workspace_size;
        I3ipc_rect rect;
} I3ipc_reply_outputs_el;

struct I3ipc_reply_outputs {
    I3ipc_reply_outputs_el* outputs;
    int outputs_size;
};

/* see https://i3wm.org/docs/ipc.html#_tree_reply */
struct I3ipc_reply_tree {
    I3ipc_node root;
};

/* see https://i3wm.org/docs/ipc.html#_marks_reply */
struct I3ipc_reply_marks {
    I3ipc_string* marks;
    int           marks_size;
};

/* see https://i3wm.org/docs/ipc.html#_bar_config_reply */
struct I3ipc_reply_bar_config_ids {
    I3ipc_string* ids;
    int ids_size;
};

/* see https://i3wm.org/docs/ipc.html#_bar_config_reply */
struct I3ipc_reply_bar_config {
    I3ipc_bar_config cfg;
};

/* see https://i3wm.org/docs/ipc.html#_version_reply */
struct I3ipc_reply_version {
    int   major;
    int   minor;
    int   patch;
    char* human_readable;
    int   human_readable_size;
    char* loaded_config_file_name;
    int   loaded_config_file_name_size;
};

/* see https://i3wm.org/docs/ipc.html#_binding_modes_reply */
struct I3ipc_reply_binding_modes {
    I3ipc_string* modes;
    int           modes_size;
};

/* see https://i3wm.org/docs/ipc.html#_config_reply */
struct I3ipc_reply_config {
    char* config;
    int   config_size;
};

/* see https://i3wm.org/docs/ipc.html#_tick_reply */
struct I3ipc_reply_tick {
    bool success;
};

/* see https://i3wm.org/docs/ipc.html#_sync_reply */
struct I3ipc_reply_sync {
    bool success;
};


enum I3ipc_event_workspace_values {
    I3IPC_WORKSPACE_CHANGE_FOCUS = 0,
    I3IPC_WORKSPACE_CHANGE_INIT,
    I3IPC_WORKSPACE_CHANGE_EMPTY,
    I3IPC_WORKSPACE_CHANGE_URGENT,
    I3IPC_WORKSPACE_CHANGE_RELOAD,
    I3IPC_WORKSPACE_CHANGE_RENAME,
    I3IPC_WORKSPACE_CHANGE_RESTORED,
    I3IPC_WORKSPACE_CHANGE_MOVE
};

/* see https://i3wm.org/docs/ipc.html#_workspace_event */
typedef struct I3ipc_event_workspace {
    int   type; /* = I3IPC_EVENT_WORKSPACE */
    char* change;
    int   change_size;
    int   change_enum;
    I3ipc_node* current;
    I3ipc_node* old;
} I3ipc_event_workspace;

enum I3ipc_event_output_values {
    I3IPC_OUTPUT_CHANGE_UNSPECIFIED = 0
};

/* see https://i3wm.org/docs/ipc.html#_output_event */
typedef struct I3ipc_event_output {
    int   type; /* = I3IPC_EVENT_OUTPUT */
    char* change;
    int   change_size;
    int   change_enum;
} I3ipc_event_output;

/* see https://i3wm.org/docs/ipc.html#_mode_event */
typedef struct I3ipc_event_mode {
    int   type; /* = I3IPC_EVENT_MODE */
    char* change;
    int   change_size;
    bool  pango_markup;
} I3ipc_event_mode;

enum I3ipc_event_window_values {
    I3IPC_WINDOW_CHANGE_NEW = 0,
    I3IPC_WINDOW_CHANGE_CLOSE,
    I3IPC_WINDOW_CHANGE_FOCUS,
    I3IPC_WINDOW_CHANGE_TITLE,
    I3IPC_WINDOW_CHANGE_FULLSCREEN_MODE,
    I3IPC_WINDOW_CHANGE_MOVE,
    I3IPC_WINDOW_CHANGE_FLOATING,
    I3IPC_WINDOW_CHANGE_URGENT,
    I3IPC_WINDOW_CHANGE_MARK
};

/* see https://i3wm.org/docs/ipc.html#_window_event */
typedef struct I3ipc_event_window {
    int   type; /* = I3IPC_EVENT_WINDOW */
    char* change;
    int   change_size;
    int   change_enum;
    I3ipc_node container;
} I3ipc_event_window;

/* see https://i3wm.org/docs/ipc.html#_barconfig_update_event */
typedef struct I3ipc_event_barconfig_update {
    int type; /* = I3IPC_EVENT_BARCONFIG_UPDATE */
    I3ipc_bar_config cfg;
} I3ipc_event_barconfig_update;

enum I3ipc_event_binding_values {
    I3IPC_BINDING_CHANGE_RUN = 0,

    I3IPC_BINDING_INPUT_TYPE_KEYBOARD = 0,
    I3IPC_BINDING_INPUT_TYPE_MOUSE
};

/* see https://i3wm.org/docs/ipc.html#_binding_event */
typedef struct I3ipc_event_binding_binding {
    char* command;
    int   command_size;
    I3ipc_string* event_state_mask;
    int           event_state_mask_size;
    int   input_code;
    char* symbol;
    int   symbol_size;
    char* input_type;
    int   input_type_size;
    int   input_type_enum;
} I3ipc_event_binding_binding;

typedef struct I3ipc_event_binding {
    int   type; /* = I3IPC_EVENT_BINDING */
    char* change;
    int   change_size;
    int   change_enum;
    I3ipc_event_binding_binding binding;
} I3ipc_event_binding;

enum I3ipc_event_shutdown_values {
    I3IPC_SHUTDOWN_CHANGE_RESTART = 0,
    I3IPC_SHUTDOWN_CHANGE_EXIT
};

/* see https://i3wm.org/docs/ipc.html#_shutdown_event */
typedef struct I3ipc_event_shutdown {
    int   type; /* = I3IPC_EVENT_SHUTDOWN */
    char* change;
    int   change_size;
    int   change_enum;
} I3ipc_event_shutdown;

/* see https://i3wm.org/docs/ipc.html#_tick_event */
typedef struct I3ipc_event_tick {
    int   type; /* = I3IPC_EVENT_TICK */
    bool  first;
    char* payload;
    int   payload_size;
} I3ipc_event_tick;

union I3ipc_event { /*
^~~~~ This is a union! Use type to determine which member is valid. */
    
    int type;
    I3ipc_event_workspace        workspace;
    I3ipc_event_output           output;
    I3ipc_event_mode             mode;
    I3ipc_event_window           window;
    I3ipc_event_barconfig_update barconfig_update;
    I3ipc_event_binding          binding;
    I3ipc_event_shutdown         shutdown;
    I3ipc_event_tick             tick;
};


/* *** Error handling *** */

/* Set the nopanic flag, return the old value.
 * If this is set, errors will not cause the program to abort. See "Error Handling"
 * in the README. */
bool i3ipc_set_nopanic(bool value);

enum I3ipc_error_codes {
    I3IPC_ERROR_CLOSED = 256,  /* Connection with i3 closed */
    I3IPC_ERROR_MALFORMED,     /* i3 sent invalid data */
    I3IPC_ERROR_IO,            /* General IO failure */    
    I3IPC_ERROR_FAILED,        /* Operation failed */
    I3IPC_ERROR_BADSTATE = -1  /* Library in error state, operation not attempted */
};

/* Return the current error state.
 * Zero indicates no error, else the value is positive and listed in I3ipc_error_codes.
 * Note that I3IPC_ERROR_BADSTATE is never returned here. */
int i3ipc_error_code(void);

/* Print an informative message describing the current error state onto stderr.
 * Each outputted line is prefixed with prefix followed by ": ".
 * prefix may be NULL, which is equivalent to "Error". 
 * If there is no error, this may panic. */
void i3ipc_error_print(char const* prefix);

/* Reset the error state and revert the library to an un-initialized state, if necessary.
 * The latter part is skipped if the error state is I3IPC_ERROR_FAILED. You can override this
 * behaviour by setting force_reinit.
 * If there is no error, this may panic. */
void i3ipc_error_reinitialize(bool force_reinit);


/* *** Low-level API *** 
 * Functions ending with _try return 0 on success and a nonzero error code on failure. 
 * Error codes are defined in I3ipc_error_codes.
 * The code of the first failure matches subsequent calls to i3ipc_error_code. While the error
 * state is set, *_try functions return I3IPC_ERROR_BADSTATE instead of performing any work.  */

typedef struct __attribute__ ((__packed__)) I3ipc_message {
    char magic[6];
    int32_t message_length; /* number of payload bytes after this struct */
    int32_t message_type; /* see I3ipc_message_type, I3ipc_reply_type and I3ipc_event_type */
    /* followed by message_length bytes of payload */
} I3ipc_message;

/* Initialise the connection to i3.
 * The connection is initialised automatically, you generally do not need to call this.
 * socketpath is the path to the i3 socket, it may be NULL.
 * If socketpath is NULL, a path is determined by calling 'i3 --get-socketpath'. */
int i3ipc_init_try(char* socketpath);

/* Send a message, receive an answer, parse the answer.
 * Same as calling i3ipc_message_try and i3ipc_parse_try in sequence. */
int i3ipc_message_and_parse_try(int message, int type, char const* payload, int payload_size, char** out_data);

/* Send a message and receive an answer.
 * Same as calling i3ipc_message_send_try and i3ipc_message_receive try in sequence. */
int i3ipc_message_try(int message_type, char const* payload, int payload_size, I3ipc_message** out_reply);

/* Send a message to i3.
 * Events and SUBSCRIBE messages use a second socket. */
int i3ipc_message_send_try(int message_type, char const* payload, int payload_size);

/* Receive the next message, its type must match message_type.
 * Events and SUBSCRIBE messages use a second socket.
 * out_reply is an output parameter, it may be NULL.
 * message_type may be I3IPC_EVENT_ANY, in which case any event or SUBSCRIBE message matches. */
int i3ipc_message_receive_try(int message_type, I3ipc_message** out_reply);

/* Receive messages until one with type message_type arrives, then return that.
 * The other messages are placed into a queue and considered for subsequent calls.
 * If the first message does not match, this allocates memory.
 * out_reply is an output parameter, it may be NULL.
 * message_type may be I3IPC_EVENT_ANY, in which case any event or SUBSCRIBE message matches. */
int i3ipc_message_receive_reorder_try(int message_type, I3ipc_message** out_reply);

/* Parse the json payload of a message.
 * message_type is the expected type of the message, or -1.
 * type_id is the id of the type of the data, see I3ipc_type_values.
 * out_data is an output parameter, it may be NULL. */
int i3ipc_parse_try(I3ipc_message* msg, int message_type, int type_id, char** out_data);

/* Print a json representation of a type to stream f.
 * type_id is the id of the type of the data, see I3ipc_type_values.
 * f may be NULL, in which case stdout will be used. */
void i3ipc_printjson(int type_id, void* obj, FILE* f);


enum I3ipc_message_type {
    I3IPC_RUN_COMMAND       =  0,
    I3IPC_GET_WORKSPACES    =  1,
    I3IPC_SUBSCRIBE         =  2,
    I3IPC_GET_OUTPUTS       =  3,
    I3IPC_GET_TREE          =  4,
    I3IPC_GET_MARKS         =  5,
    I3IPC_GET_BAR_CONFIG    =  6,
    I3IPC_GET_VERSION       =  7,
    I3IPC_GET_BINDING_MODES =  8,
    I3IPC_GET_CONFIG        =  9,
    I3IPC_SEND_TICK         = 10,
    I3IPC_SYNC              = 11,
    I3IPC_MESSAGE_TYPE_COUNT
};

enum I3ipc_reply_type {
    I3IPC_REPLY_COMMAND       =  0,
    I3IPC_REPLY_WORKSPACES    =  1,
    I3IPC_REPLY_SUBSCRIBE     =  2,
    I3IPC_REPLY_OUTPUTS       =  3,
    I3IPC_REPLY_TREE          =  4,
    I3IPC_REPLY_MARKS         =  5,
    I3IPC_REPLY_BAR_CONFIG    =  6,
    I3IPC_REPLY_VERSION       =  7,
    I3IPC_REPLY_BINDING_MODES =  8,
    I3IPC_REPLY_CONFIG        =  9,
    I3IPC_REPLY_TICK          = 10,
    I3IPC_REPLY_SYNC          = 11,
    I3IPC_REPLY_TYPE_COUNT
};

enum I3ipc_event_type {
    I3IPC_EVENT_WORKSPACE = -2147483648, /* 0 ^ 1<<31 */
    I3IPC_EVENT_OUTPUT,                  /* 1 ^ 1<<31 */
    I3IPC_EVENT_MODE,                    /* 2 ^ 1<<31 */
    I3IPC_EVENT_WINDOW,                  /* 3 ^ 1<<31 */
    I3IPC_EVENT_BARCONFIG_UPDATE,        /* 4 ^ 1<<31 */
    I3IPC_EVENT_BINDING,                 /* 5 ^ 1<<31 */
    I3IPC_EVENT_SHUTDOWN,                /* 6 ^ 1<<31 */
    I3IPC_EVENT_TICK,                    /* 7 ^ 1<<31 */
    I3IPC_EVENT_TYPE_END,                 
    I3IPC_EVENT_TYPE_BEGIN = I3IPC_EVENT_WORKSPACE,
    I3IPC_EVENT_ANY = -2 /* matches any event or SUBSCRIBE messages */
};


enum I3ipc_type_values {
    /* Primitive types */
    I3IPC_TYPE_BOOL,       /* bool */
    I3IPC_TYPE_CHAR,       /* char, only used for strings, as array */
    I3IPC_TYPE_INT,        /* int */
    I3IPC_TYPE_FLOAT,      /* float */
    I3IPC_TYPE_SIZET,      /* size_t */

    /* Basic types. These are used in multiple places. */
    I3IPC_TYPE_RECT,       /* I3ipc_rect */
    I3IPC_TYPE_STRING,     /* I3ipc_string */
    I3IPC_TYPE_NODE,       /* I3ipc_rect */
    I3IPC_TYPE_BAR_CONFIG, /* I3ipc_bar_config */

    /* Inner types. All types are named, and these are only used once and declared inline. */
    I3IPC_TYPE_NODE_WINDOW_PROPERTIES,  /* struct I3ipc_node_window_properties */
    I3IPC_TYPE_BAR_CONFIG_COLORS,       /* struct I3ipc_bar_config_colors  */
    I3IPC_TYPE_REPLY_COMMAND_EL,        /* struct I3ipc_reply_command_el  */
    I3IPC_TYPE_REPLY_OUTPUTS_EL,        /* struct I3ipc_reply_outputs_el  */
    I3IPC_TYPE_REPLY_WORKSPACES_EL,     /* struct I3ipc_reply_workspaces_el */
    I3IPC_TYPE_EVENT_BINDING_BINDING,   /* struct I3ipc_event_binding_binding */

    /* Message reply types */
    I3IPC_TYPE_REPLY_COMMAND,           /* I3ipc_reply_command */
    I3IPC_TYPE_REPLY_WORKSPACES,        /* I3ipc_reply_workspaces */
    I3IPC_TYPE_REPLY_SUBSCRIBE,         /* I3ipc_reply_subscribe */
    I3IPC_TYPE_REPLY_OUTPUTS,           /* I3ipc_reply_outputs */
    I3IPC_TYPE_REPLY_TREE,              /* I3ipc_reply_tree */
    I3IPC_TYPE_REPLY_MARKS,             /* I3ipc_reply_marks */
    I3IPC_TYPE_REPLY_BAR_CONFIG_IDS,    /* I3ipc_reply_bar_config_ids */
    I3IPC_TYPE_REPLY_BAR_CONFIG,        /* I3ipc_reply_bar_config */
    I3IPC_TYPE_REPLY_VERSION,           /* I3ipc_reply_version */
    I3IPC_TYPE_REPLY_BINDING_MODES,     /* I3ipc_reply_binding_modes */
    I3IPC_TYPE_REPLY_CONFIG,            /* I3ipc_reply_config */
    I3IPC_TYPE_REPLY_TICK,              /* I3ipc_reply_tick */
    I3IPC_TYPE_REPLY_SYNC,              /* I3ipc_reply_sync */

    /* Event types */
    I3IPC_TYPE_EVENT,                   /* I3ipc_event */
    I3IPC_TYPE_EVENT_WORKSPACE,         /* I3ipc_event_workspace */
    I3IPC_TYPE_EVENT_OUTPUT,            /* I3ipc_event_output */
    I3IPC_TYPE_EVENT_MODE,              /* I3ipc_event_mode */
    I3IPC_TYPE_EVENT_WINDOW,            /* I3ipc_event_window */
    I3IPC_TYPE_EVENT_BARCONFIG_UPDATE,  /* I3ipc_event_barconfig_update */
    I3IPC_TYPE_EVENT_BINDING,           /* I3ipc_event_binding */
    I3IPC_TYPE_EVENT_SHUTDOWN,          /* I3ipc_event_shutdown */
    I3IPC_TYPE_EVENT_TICK,              /* I3ipc_event_tick */

    I3IPC_TYPE_COUNT,
    I3IPC_TYPE_PRIMITIVE_COUNT = I3IPC_TYPE_RECT
};

#endif /* I3IPC_INCLUDE_I3IPC_H */

#ifdef I3IPC_IMPLEMENTATION

#include <assert.h>
#include <alloca.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>

#ifndef I3IPC_ALIGNOF

#ifdef __cplusplus
template<typename Type> struct I3ipc__align { char c; Type t; };
#define I3IPC_ALIGNOF(T) offsetof(I3ipc__align<T>, t)
#else
#define I3IPC_ALIGNOF(T) offsetof(struct {char c; T t;}, t)
#endif

#endif /* I3IPC_ALIGNOF */


#ifndef I3IPC_ANONYMOUS_UNION

/* TODO: This could be more generous */
#if __STDC_VERSION__ >= 201112L
#define I3IPC_ANONYMOUS_UNION 1
#else
#define I3IPC_ANONYMOUS_UNION 0
#endif

#endif /* I3IPC_ANONYMOUS_UNION */



static bool i3ipc__globals_initialized;
static FILE* i3ipc__err;
static char* i3ipc__error_buf;
static size_t i3ipc__error_buf_size;

enum I3ipc_context_state {
    I3IPC_STATE_UNINITIALIZED = 0,
    I3IPC_STATE_READY         = 1,
    I3IPC_STATE_ERROR_BEGIN   = I3IPC_ERROR_CLOSED
    /* error codes in I3ipc_error_codes are valid states */
};

enum I3ipc_context_buffers {
    I3IPC_CONTEXT_MSG,
    I3IPC_CONTEXT_PARSE,
    I3IPC_CONTEXT_ALLOCS,
    I3IPC_CONTEXT_REORDER,
    I3IPC_CONTEXT_JSON,
    I3IPC_CONTEXT_PAYLOAD,
    I3IPC_CONTEXT_BUFFER_SIZE
};

typedef struct I3ipc_context {
    int state;
    int sock;
    int sock_events;

    char* buffers[I3IPC_CONTEXT_BUFFER_SIZE];
    size_t buffer_sizes[I3IPC_CONTEXT_BUFFER_SIZE];
    
    bool nopanic;
    bool staticalloc;
    bool debug_do_not_write_messages;
    bool debug_nodata_is_error;
    int loglevel;

    int events_queued;
} I3ipc_context;

/* This is (and should be) zero-initialised */
static I3ipc_context i3ipc__global_context;

void i3ipc_error_print(char const* prefix) {
    if (prefix == NULL) prefix = "Error";
    
    fflush(i3ipc__err);
    size_t last = 0;
    for (size_t i = 0; i < i3ipc__error_buf_size; ++i) {
        if (i3ipc__error_buf[i] == '\n') {
            fprintf(stderr, "%s: ", prefix);
            fwrite(i3ipc__error_buf + last, 1, i - last, stderr);
            fputs("\n", stderr);
            last = i+1;
        }
    }
    fseek(i3ipc__err, 0, SEEK_SET);
}
void i3ipc__error_clearbuf(void) {
    fflush(i3ipc__err);
    fseek(i3ipc__err, 0, SEEK_SET);
}

void i3ipc__error_errno(char const* message) {
    if (!i3ipc__err) return; /* this happens for some testing configurations */
    fprintf(i3ipc__err, "%s\n%s\n", strerror(errno), message);
}

int i3ipc_error_code(void) {
    I3ipc_context* context = &i3ipc__global_context;
    if (context->state < I3IPC_STATE_ERROR_BEGIN) return 0;
    return context->state;
}

void i3ipc_error_reinitialize(bool force_reinit) {
    int code = i3ipc_error_code();
    assert(code);
    
    I3ipc_context* context = &i3ipc__global_context;
    if (code != I3IPC_ERROR_FAILED || force_reinit) {
        /* full un-initialisation */
        context->state = I3IPC_STATE_UNINITIALIZED;
        close(context->sock);
        context->sock = 0;
        close(context->sock_events);
        context->sock_events = 0;
        context->events_queued = 0;
    } else {
        /* only reset error state */
        context->state = I3IPC_STATE_READY;
    }
}

int i3ipc__error_handle(int code) {
    I3ipc_context* context = &i3ipc__global_context;
    if (code) {
        if (code != I3IPC_ERROR_BADSTATE) {
            context->state = code;
        }
        if (!context->nopanic) {
            if (context->loglevel >= 0) {
                i3ipc_error_print("Error");
            } else {
                i3ipc__error_clearbuf();
            }
            abort();
        }
    }
    return code;
}

bool i3ipc_set_staticalloc(bool value) {
    I3ipc_context* context = &i3ipc__global_context;
    bool prev = context->staticalloc;
    context->staticalloc = value;
    return prev;
}

int i3ipc_set_loglevel(int value) {
    I3ipc_context* context = &i3ipc__global_context;
    int prev = context->loglevel;
    context->loglevel = value;
    return prev;
}

bool i3ipc_set_nopanic(bool value) {
    I3ipc_context* context = &i3ipc__global_context;
    bool prev = context->nopanic;
    context->nopanic = value;
    if (!value && i3ipc_error_code()) {
        fprintf(i3ipc__err, "while enabling panic on error (triggering on stored error state)\n");
        if (context->loglevel >= 0) {
            i3ipc_error_print("Error");
        } else {
            i3ipc__error_clearbuf();
        }
        abort();        
    }
    return prev;
}

int i3ipc_message_fd(void) {
    I3ipc_context* context = &i3ipc__global_context;
    return context->sock;
}
int i3ipc_event_fd(void) {
    I3ipc_context* context = &i3ipc__global_context;
    return context->sock_events;
}

int i3ipc__socketpath_cmd_try(char** out_path) {
    assert(out_path);
    int pipefd[2];
    if (pipe(pipefd)) {
        i3ipc__error_errno("while calling pipe()");
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (close(pipefd[0])) abort();
        dup2(pipefd[1], STDOUT_FILENO);
        
        char const* argv[] = {"i3", "--get-socketpath", NULL};
        execvp("i3", (char**)argv);
        abort(); /* reached only in case of error */
    } else if (pid == -1) {
        i3ipc__error_errno("while calling fork()");
        return 2;
    }

    if (close(pipefd[1])) {
        i3ipc__error_errno("while closing unused end of pipe");
        return 3;
    }

    size_t off = 0;
    char* buf = NULL;
    size_t buf_size = 0;
    const size_t space_min = 64;
    int rcode = -1;

    while (true) {
        if (buf_size - off < space_min) {
            buf_size *= 2;
            if (buf_size - off < space_min) buf_size = off + space_min;
            buf = (char*)realloc(buf, buf_size);
        }
        
        ssize_t bytes_read = read(pipefd[0], buf+off, buf_size-off);
        if (bytes_read == -1) {
            i3ipc__error_errno("while calling read()");
            rcode = 4; goto cleanup;
        }
        if (bytes_read == 0) break;
        
        off += bytes_read;
    }

    siginfo_t info;
    memset(&info, 0, sizeof(info));
    {int code = waitid(P_PID, pid, &info, WEXITED);
    if (code == -1) {
        i3ipc__error_errno("while calling waitid()");
        rcode = 5; goto cleanup;
    } else if (info.si_code == CLD_EXITED && info.si_status) {
        fprintf(i3ipc__err, "child i3 returned non-zero exit code %d\n", info.si_status);
        rcode = 6; goto cleanup;
    } else if (info.si_code == CLD_KILLED || info.si_code == CLD_DUMPED) {
        fprintf(i3ipc__err, "child i3 exited abnormally\n");
        rcode = 7; goto cleanup;
    }}
    
    if (off == 0 || buf[off-1] != '\n') {
        fprintf(i3ipc__err, "i3 output does not end with newline\n");
        rcode = 8;
        goto cleanup;
    }
    buf[off-1] = 0; /* overwrite the newline at the end */

    *out_path = buf;
    return 0;

  cleanup:
    free(buf);
    return rcode;
}

int i3ipc__socket_open_try(char* socketpath, int* out_sock, char** out_socketpath) {
    assert(out_sock);
    
    char* socketpath_to_free = NULL;
    int rcode = -1;
    if (!socketpath) {
        if (i3ipc__socketpath_cmd_try(&socketpath_to_free)) {
            rcode = 1;
            goto done;
        }
        socketpath = socketpath_to_free;
    }
    if (out_socketpath) {
        *out_socketpath = socketpath;
        socketpath_to_free = NULL;
    }

    {int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        fprintf(i3ipc__err, "%s\nwhile opening unix socket\n", strerror(errno));
        rcode = 2;
        goto cleanup;
    }

    struct sockaddr_un sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sun_family = AF_UNIX;
    {size_t socketpath_size = strlen(socketpath);
    if (socketpath_size >= sizeof(sock_addr.sun_path)) {
        fprintf(i3ipc__err, "socket pathname has length %ld, which exceeds the maximum size %ld\n",
        (long)socketpath_size, (long)(sizeof(sock_addr.sun_path)-1));
        rcode = 3;
        goto cleanup;
    }

    memcpy(&sock_addr.sun_path, socketpath, socketpath_size);
    sock_addr.sun_path[socketpath_size] = 0;}

    {int code = connect(sock, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if (code) {
        fprintf(i3ipc__err, "%s\nwhile binding unix socket to '%s'\n", strerror(errno), socketpath);
        rcode = 4;
        goto cleanup;
    }}

    *out_sock = sock;}
    rcode = 0;

  cleanup:
    free(socketpath_to_free);
  done:
    return rcode;
}


static char const* const i3ipc__global_message_type_name[] = {
    "run_command",
    "get_workspaces",
    "subscribe",
    "get_outputs",
    "get_tree",
    "get_marks",
    "get_bar_config",
    "get_version",
    "get_binding_modes",
    "get_config",
    "send_tick",
    "sync"
};
static char const* const i3ipc__global_reply_type_name[] = {
    "command",
    "workspaces",
    "subscribe",
    "outputs",
    "tree",
    "marks",
    "bar_config",
    "version",
    "binding_modes",
    "config",
    "tick",
    "sync"
};
static char const* const i3ipc__global_event_type_name[] = {
    "workspace",
    "output",
    "mode",
    "window",
    "barconfig_update",
    "binding",
    "shutdown",
    "tick"
};


void i3ipc__init_globals();

int i3ipc_init_try(char* socketpath) {
    if (!i3ipc__globals_initialized) {
        i3ipc__init_globals();        
    }
    
    I3ipc_context* context = &i3ipc__global_context;
    if (context->state == I3IPC_STATE_READY) return 0;
    if (i3ipc_error_code()) return I3IPC_ERROR_BADSTATE;

    char* socketpath_to_free = NULL;
    {int code = i3ipc__socket_open_try(socketpath, &context->sock, &socketpath_to_free);
    if (code) goto error_free;}

    if (!socketpath) {
        assert(socketpath_to_free);
        socketpath = socketpath_to_free;
    }

    {int code = i3ipc__socket_open_try(socketpath, &context->sock_events, NULL);
    if (code) goto error_free;}
    
    free(socketpath_to_free);
    context->state = I3IPC_STATE_READY;
    return 0;

  error_free:
    free(socketpath_to_free);
    context->state = I3IPC_ERROR_CLOSED;
    return I3IPC_ERROR_CLOSED;
}

enum I3ipc_write_all_code {
    I3IPC_WRITE_ALL_SUCCESS = 0,
    I3IPC_WRITE_ALL_ERROR = 101,
    I3IPC_WRITE_ALL_EOF = 102,
    I3IPC_WRITE_ALL_WOULDBLOCK = 103
};

int i3ipc__write_all_try(int fd, char* buf, ssize_t buf_size) {
    while (buf_size > 0) {
        ssize_t bytes_written = write(fd, buf, buf_size);
        if (bytes_written == -1) {
            if (errno == EPIPE) {
                fprintf(i3ipc__err, "eof while writing bytes (%ld left to write)\n", (long)buf_size);
                return I3IPC_WRITE_ALL_EOF;
            } else {
                bool wouldblock = errno == EWOULDBLOCK || errno == EAGAIN;
                i3ipc__error_errno("while calling write()");
                return wouldblock ? I3IPC_WRITE_ALL_WOULDBLOCK : I3IPC_WRITE_ALL_ERROR;
            }
        }
        assert(bytes_written > 0);
        buf_size -= bytes_written;
        buf      += bytes_written;
    }

    assert(buf_size == 0);
    return 0;
}

enum I3ipc_read_all_code {
    I3IPC_READ_ALL_SUCCESS = 0,
    I3IPC_READ_ALL_ERROR = 201,
    I3IPC_READ_ALL_EOF = 202,
    I3IPC_READ_ALL_WOULDBLOCK = 203
};

int i3ipc__read_all_try(int fd, char* buf, ssize_t buf_size) {
    while (buf_size > 0) {
        ssize_t bytes_read = read(fd, buf, buf_size);
        if (bytes_read == -1) {
            bool wouldblock = errno == EWOULDBLOCK || errno == EAGAIN;
            i3ipc__error_errno("while calling read()");
            return wouldblock ? I3IPC_READ_ALL_WOULDBLOCK : I3IPC_READ_ALL_ERROR;
        }
        if (bytes_read == 0) {
            /* Note that buf_size > 0 due to loop condition */
            fprintf(i3ipc__err, "unexpected eof (%ld bytes left to read)\n", (long)buf_size);
            return I3IPC_READ_ALL_EOF;
        }
        buf_size -= bytes_read;
        buf      += bytes_read;
    }
    
    assert(buf_size == 0);
    return 0;
}

void i3ipc__context_reserve(I3ipc_context* context, int buf_id, size_t size_next, void** out_ptr) {
    assert(context);
    assert(0 <= buf_id && buf_id < I3IPC_CONTEXT_BUFFER_SIZE);
    
    char** buf = &context->buffers[buf_id];
    size_t* buf_size = &context->buffer_sizes[buf_id];

    if (*buf_size < size_next) {
        *buf_size *= 2;
        if (*buf_size < size_next) {
            *buf_size = size_next;
        }
        *buf = (char*)realloc(*buf, *buf_size);
    }
    if (out_ptr) *out_ptr = *buf;
}

int i3ipc__message_type_to_socket(I3ipc_context* context, int message_type) {
    if (message_type == I3IPC_SUBSCRIBE) {
        return context->sock_events;
    } else if (0 <= message_type && message_type < I3IPC_MESSAGE_TYPE_COUNT) {
        return context->sock;
    } else if (I3IPC_EVENT_TYPE_BEGIN <= message_type && message_type < I3IPC_EVENT_TYPE_END) {
        /* should not really happen, events are usually not received by type */
        return context->sock_events;
    } else if (message_type == I3IPC_EVENT_ANY) {
        return context->sock_events;
    } else {
        assert(false);
    }
}

char const* i3ipc__message_type_str(int message_type, bool is_reply) {
    assert((int)I3IPC_MESSAGE_TYPE_COUNT == (int)I3IPC_REPLY_TYPE_COUNT);
    if (0 <= message_type && message_type < I3IPC_MESSAGE_TYPE_COUNT) {
        return is_reply ? i3ipc__global_message_type_name[message_type]
            : i3ipc__global_reply_type_name[message_type];
    } else if (1<<31 <= message_type && message_type <= I3IPC_EVENT_TYPE_END) {
        return i3ipc__global_event_type_name[message_type ^ 1<<31];
    } else if (message_type == I3IPC_EVENT_ANY) {
        return "<any-event-or-subscribe>";
    } else {
        return "invalid";
    }
}

int i3ipc_message_send_try(int message_type, char const* payload, int payload_size) {
    assert(0 <= message_type && message_type < I3IPC_MESSAGE_TYPE_COUNT);
    assert(-1 <= payload_size);

    if (payload_size == -1) {
        payload_size = payload ? strlen(payload) : 0;
    }

    I3ipc_context* context = &i3ipc__global_context;
    {int code = i3ipc_init_try(NULL);
    if (code) return code;}

    int sock = i3ipc__message_type_to_socket(context, message_type);

    I3ipc_message* msg;
    i3ipc__context_reserve(context, I3IPC_CONTEXT_MSG,
        sizeof(I3ipc_message) + payload_size, (void**)&msg);

    memcpy(&msg->magic, "i3-ipc", 6);
    msg->message_type = message_type;

    if (message_type == I3IPC_RUN_COMMAND || message_type == I3IPC_SUBSCRIBE
            || message_type == I3IPC_SEND_TICK || message_type == I3IPC_SYNC
            || message_type == I3IPC_GET_BAR_CONFIG) {
        msg->message_length = payload_size;
        if (payload) memcpy(msg + 1, payload, payload_size);
    } else {
        msg->message_length = 0;
        assert(payload_size == 0);
    }

    if (context->loglevel >= 1) {
        fprintf(stderr, "Debug: Sending message with type %s(%x) to ",
            i3ipc__message_type_str(msg->message_type, false), msg->message_type);
        if (sock == context->sock) {
            fprintf(stderr, "message socket");
        } else if (sock == context->sock_events) {
            fprintf(stderr, "event socket");
        } else {
            assert(false);
        }
        fprintf(stderr, ", length %u, payload ", msg->message_length);
        fwrite(msg + 1, 1, msg->message_length, stderr);
        fputs("\n", stderr);
    }

    if (!context->debug_do_not_write_messages) {
        int code = i3ipc__write_all_try(sock, (char*)msg, sizeof(*msg) + msg->message_length);
        if (code == I3IPC_WRITE_ALL_EOF) {
            i3ipc__error_clearbuf();
            return i3ipc__error_handle(I3IPC_ERROR_CLOSED);
        } else if (code) {
            fprintf(i3ipc__err, "while sending message to i3\n");
            return i3ipc__error_handle(I3IPC_ERROR_IO);
        }
    }

    return 0;
}


int i3ipc_message_receive_try(int message_type, I3ipc_message** out_reply) {
    I3ipc_context* context = &i3ipc__global_context;
    {int code = i3ipc_init_try(NULL);
    if (code) return code;}
    
    int sock = i3ipc__message_type_to_socket(context, message_type);
    
    I3ipc_message* msg;
    i3ipc__context_reserve(context, I3IPC_CONTEXT_MSG, sizeof(*msg), (void**)&msg);

    {int code = i3ipc__read_all_try(sock, (char*)msg, sizeof(*msg));
    if (!code) {
        if (msg->message_length < 0) {
            fprintf(i3ipc__err, "i3 sent message with negative length (size %d)\n", msg->message_length);
            return i3ipc__error_handle(I3IPC_ERROR_MALFORMED);            
        }
        
        size_t size = sizeof(*msg) + msg->message_length + 1;
#ifdef I3IPC_FUZZ
        size_t size_max = 2048;
#else
        size_t size_max = 256 * 1024 * 1024;
#endif
        if (size > size_max) {
            fprintf(i3ipc__err, "i3 sent too-long message (size %lu, max is %lu)\n",
                (long)size, (long)size_max);
            return i3ipc__error_handle(I3IPC_ERROR_MALFORMED);            
        }
        i3ipc__context_reserve(context, I3IPC_CONTEXT_MSG, size, (void**)&msg);
        
        code = i3ipc__read_all_try(sock, (char*)(msg + 1), msg->message_length);
    }
    
    if (code == I3IPC_READ_ALL_EOF) {
        i3ipc__error_clearbuf();
        return i3ipc__error_handle(I3IPC_ERROR_CLOSED);
    } else if (code == I3IPC_READ_ALL_WOULDBLOCK && context->debug_nodata_is_error) {
        i3ipc__error_clearbuf();
        return i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
    } else if (code) {
        fprintf(i3ipc__err, "while reading message from i3\n");
        return i3ipc__error_handle(I3IPC_ERROR_IO);
    }}

    ((char*)(msg + 1))[msg->message_length] = 0;

    bool matches = false;
    if (msg->message_type == message_type) {
        matches = true;
    } else if (message_type == I3IPC_EVENT_ANY) {
        if (msg->message_type == I3IPC_REPLY_SUBSCRIBE) {
            matches = true;
        } else if (I3IPC_EVENT_TYPE_BEGIN <= msg->message_type && msg->message_type < I3IPC_EVENT_TYPE_END) {
            matches = true;
        }
    }
    
    if (!matches) {
        fprintf(i3ipc__err, "message type does not match, expected %s(%x), got %s(%x)\n",
            i3ipc__message_type_str(message_type, true), message_type,
            i3ipc__message_type_str(msg->message_type, true), msg->message_type);
        return i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
    }

    if (context->loglevel >= 1) {
        fprintf(stderr, "Debug: Receiving message with type %s(%x) from ",
            i3ipc__message_type_str(msg->message_type, true), msg->message_type);
        if (sock == context->sock) {
            fprintf(stderr, "message socket");
        } else if (sock == context->sock_events) {
            fprintf(stderr, "event socket");
        } else {
            assert(false);
        }
        fprintf(stderr, ", length %u, payload ", msg->message_length);
        fwrite(msg + 1, 1, msg->message_length, stderr);
        fputs("\n", stderr);
    }

    if (out_reply) *out_reply = msg;
    return 0;
}

int i3ipc_message_try(int message_type, char const* payload, int payload_size, I3ipc_message** out_reply) {
    {int code = i3ipc_message_send_try(message_type, payload, payload_size);
    if (code) return code;}
    
    {int code = i3ipc_message_receive_try(message_type, out_reply);
    if (code) return code;}
    
    return 0;
}

int i3ipc_message_receive_reorder_try(int message_type, I3ipc_message** out_reply) {
    assert(out_reply);

    I3ipc_context* context = &i3ipc__global_context;
    {int code = i3ipc_init_try(NULL);
    if (code) return code;}
    
    I3ipc_message** buf = (I3ipc_message**)context->buffers[I3IPC_CONTEXT_REORDER];
    
    for (int i = 0;; ++i) {
        if (i < context->events_queued) {
            if (message_type == I3IPC_EVENT_ANY || buf[i]->message_type == message_type) {
                I3ipc_message* msg;
                size_t size = sizeof(*msg) + buf[i]->message_length;
                i3ipc__context_reserve(context, I3IPC_CONTEXT_MSG, size, (void**)&msg);
                memcpy(msg, buf[i], size);
                
                free(buf[i]);
                for (int j = i; j+1 < context->events_queued; ++j) {
                    buf[j] = buf[j+1];
                }
                buf[context->events_queued-1] = 0;
                --context->events_queued;

                *out_reply = msg;
                return 0;
            }
        } else {
            I3ipc_message* msg;
            int code = i3ipc_message_receive_try(I3IPC_EVENT_ANY, &msg);
            if (code) return code;

            if (message_type == I3IPC_EVENT_ANY || msg->message_type == message_type) {
                *out_reply = msg;
                return 0;
            }

            assert(i == context->events_queued);
            ++context->events_queued;
            i3ipc__context_reserve(context, I3IPC_CONTEXT_REORDER, (i+1)*sizeof(buf[0]), (void**)&buf);
    
            size_t size = sizeof(*msg) + msg->message_length;
            buf[i] = (I3ipc_message*)calloc(1, size);
            memcpy(buf[i], msg, size);            
        }
    }
}

#if I3IPC_ANONYMOUS_UNION

typedef struct I3ipc_json_token {
    int type;
    union {
        struct { char* str; int str_size; };
        struct { double number; size_t number_long; };
        bool flag;
    };
} I3ipc_json_token;

#else

typedef struct I3ipc_json_token {
    int type;
    char* str; int str_size;
    double number; size_t number_long;
    bool flag;
} I3ipc_json_token;

#endif

typedef struct I3ipc_json_state {
    char* cur;
    int left;
    bool err_flag;
    bool nomodify_flag;
    I3ipc_json_token* tokens;
    int tokens_size;
    int tokens_capacity;
    int tokens_cur;
} I3ipc_json_state;

enum I3ipc_json_token_type {
    /* One character tokens are encoded as themselves */
    I3IPC_JSON_BOOL = 256,
    I3IPC_JSON_NULL,
    I3IPC_JSON_STRING,
    I3IPC_JSON_NUMBER,
    I3IPC_JSON_TOKEN_MAX
};

int i3ipc__json_scan_number(I3ipc_json_state* state, double* out_val, size_t* out_val_int) {
    /* This a simple parser, i.e. no correct rounding, and no exponents */
    int length = state->left;
    for (int i = 0; i < state->left; ++i) {
        char ic = state->cur[i];
        bool isdigit = '0' <= ic && ic <= '9';
        bool isother = ic == '-' || ic == '.' || ic == '+' || ic == 'e' || ic == 'E';
        if (!isdigit && !isother) {
            length = i;
            break;
        }
    }
    if (length >= 256) {
        fprintf(i3ipc__err, "number with length %d in json, this is too long\n", length);
        return 1;
    }

    char* c = (char*)alloca(length + 1);
    memcpy(c, state->cur, length);
    c[length] = 0;
    state->cur += length;
    state->left -= length;

    bool flipsign = 0;
    if (*c == '-') {
        flipsign = 1;
        ++c;
    }

    double val = 0;
    size_t val_int = 0;
    for (; '0' <= *c && *c <= '9'; ++c) {
        val = 10.0 * val + (double)(*c - '0');
        val_int = 10 * val_int + (*c - '0');
    }
    if (*c == '.') {
        ++c;
        unsigned long num = 0;
        unsigned long den = 1;
        for (int i = 0; i < 19 && '0' <= *c && *c <= '9'; ++i, ++c) {
            num = 10*num + *c - '0';
            den *= 10;
        }
        while ('0' <= *c && *c <= '9') ++c;
        val += (double)num / (double)den;
    }

    if (*c) {
        /* Exponents are not supported, i3 should not do this */
        fprintf(i3ipc__err, "found json number with exponent, this is unsupported\n");
        return 1;
    }
    
    if (out_val) *out_val = flipsign ? -val : val;
    if (out_val_int) *out_val_int = flipsign ? -val_int : val_int;
    return 0;
}

int i3ipc__json_scan_string(I3ipc_json_state* state) {
    char* p = state->cur;
    int i;
    for (i = 1; i < state->left;) {
        if (state->cur[i] == '"') {
            ++i;
            break;
        } else if (state->cur[i] == '\\') {
            char ic;
            switch (i+1 < state->left ? state->cur[i+1] : 0) {
            case '"':  ic = '"';  break;
            case '\\': ic = '\\'; break;
            case '/':  ic = '/';  break;
            case 'b':  ic = '\b'; break;
            case 'f':  ic = '\f'; break;
            case 'n':  ic = '\n'; break;
            case 'r':  ic = '\r'; break;
            case 't':  ic = '\t'; break;
            case 'u':  ic = 'u'; break;
            default:   ic = 0;    break;
            }
            if (ic == 'u') {
                if (i+5 < state->left) {
                    int cp = 0;
                    bool valid = true;
                    for (int j = i+2; j <= i+5; ++j) {
                        char jc = state->cur[j];
                        int val = '0' <= jc && jc <= '9' ? jc - '0' :
                                  'A' <= jc && jc <= 'F' ? 10 + jc - 'A' :
                                  'a' <= jc && jc <= 'f' ? 10 + jc - 'a' : -1;
                        valid &= val != -1;
                        cp = cp * 16 + val;
                    }

                    if (valid) {
                        i += 6;
                        
                        /* utf-8 encoding */
                        if (cp <= 0x7f) {
                            *p++ = cp;
                        } else if (cp <= 0x7ff) {
                            *p++ = 0xc0 | cp >> 6;
                            *p++ = 0x80 | (cp & 0x3f);
                        } else if (cp <= 0xffff) {
                            *p++ = 0xe0 | cp >> 12;
                            *p++ = 0x80 | (cp >> 6 & 0x3f);
                            *p++ = 0x80 | (cp      & 0x3f);
                        } else if (cp <= 0x10ffff) {
                            *p++ = 0xf0 | cp >> 18;
                            *p++ = 0x80 | (cp >> 12 & 0x3f);
                            *p++ = 0x80 | (cp >>  6 & 0x3f);
                            *p++ = 0x80 | (cp       & 0x3f);
                        } else {
                            /* invalid unicode codepoint */
                            *p++ = (char)0xef;
                            *p++ = (char)0xbf;
                            *p++ = (char)0xbd;
                        }
                        continue;
                    }
                }
            } else if (ic) {
                i += 2;
                *p++ = ic;
                continue;
            }
        }
        *p++ = state->cur[i];
        i += 1;
    }

    int length = p - state->cur;
    *p++ = 0;
    state->cur += i;
    state->left -= i;
    return length;
}

int i3ipc__json_scan_token(I3ipc_json_state* state, I3ipc_json_token* out_tok) {
    assert(state && out_tok);
    I3ipc_json_token tok;
    memset(&tok, 0, sizeof(tok));

    while (state->left) {
        char c = *state->cur;
        if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r')) break;
        ++state->cur;
        --state->left;
    }

    if (state->left <= 0) {
        tok.type = 0;
        *out_tok = tok;
        return 0;
    }
    
    char c = *state->cur;
    int move = 0;
    if (c == '[' || c == ']' || c == '{' || c == '}' || c == ':' || c == ',') {
        tok.type = c;
        move = 1;
    } else if (c == 't') { /* true */
        if (state->left < 4 || memcmp(state->cur, "true", 4)) return 3;
        move = 4;
        tok.type = I3IPC_JSON_BOOL;
        tok.flag = true;
    } else if (c == 'f') { /* false */
        if (state->left < 5 || memcmp(state->cur, "false", 4)) return 4;
        move = 5;
        tok.type = I3IPC_JSON_BOOL;
        tok.flag = false;
    } else if (c == 'n') { /* null */
        if (state->left < 4 || memcmp(state->cur, "null", 4)) return 5;
        move = 4;
        tok.type = I3IPC_JSON_NULL;
    } else if (c == '"') {
        tok.type = I3IPC_JSON_STRING;
        tok.str = state->cur;
        tok.str_size = i3ipc__json_scan_string(state);
    } else if (c == '-' || ('0' <= c && c <= '9')) {
        tok.type = I3IPC_JSON_NUMBER;
        if (i3ipc__json_scan_number(state, &tok.number, &tok.number_long)) return 1;
    } else {
        /* Invalid token, i3 should not do this */
        fprintf(i3ipc__err, "Invalid character '%c' (code %d) at start of token\n", c, (int)c);
        return 2;
    }

    state->cur  += move;
    state->left -= move;
    *out_tok = tok;
    return 0;
}

int i3ipc__json_scan(I3ipc_context* context, I3ipc_json_state* state) {
    while (true) {
        I3ipc_json_token tok;
        if (i3ipc__json_scan_token(state, &tok)) return 1;

        i3ipc__context_reserve(context, I3IPC_CONTEXT_JSON,
            (state->tokens_size+1) * sizeof(state->tokens[0]), (void**)&state->tokens);

        state->tokens[state->tokens_size++] = tok;
        if (tok.type == 0) break;
    }
    return 0;
}

I3ipc_json_token i3ipc__json_pop(I3ipc_json_state* state) {
    assert(state->tokens_cur < state->tokens_size);
    return state->tokens[state->tokens_cur++];
}
I3ipc_json_token i3ipc__json_peek(I3ipc_json_state* state) {
    assert(state->tokens_cur < state->tokens_size);
    return state->tokens[state->tokens_cur];
}

bool i3ipc__json_trymatchany(I3ipc_json_state* state, int type[], int type_size, I3ipc_json_token* out_tok) {
    assert(type);
    assert(type_size > 0);
    
    I3ipc_json_token tok = i3ipc__json_peek(state);
    if (out_tok) *out_tok = tok;
    for (int i = 0; i < type_size; ++i) {
        if (tok.type == type[i]) {
            i3ipc__json_pop(state);
            return true;
        }
    }

    return false;
}

static char const* const i3ipc__global_token_type_name[] = {
    "bool", "null", "string", "number"
};

size_t i3ipc__printjson_string(FILE* f, char const* str, int str_size, bool for_human) {
    assert(f && (str || str_size == 0));

    bool flag_dots = false;
    if (str_size == -1) str_size = strlen(str);
    if (str_size > 200 && for_human) {
        str_size = 31;
        flag_dots = true;
    }
    
    size_t written = 0;
    fputc('"', f); ++written;
    for (int i = 0; i < str_size; ++i) {
        if (str[i] == '\\' || str[i] == '\"') {
            fputc('\\', f); ++written;
            fputc(str[i], f); ++written;
        } else if (str[i] == '\n') {
            fputs("\\n", f); ++written;
        } else if (0 <= str[i] && str[i] < ' ') {
            written += fprintf(f, "\\u%04x", (int)(unsigned char)str[i]);
        } else {
            fputc(str[i], f); ++written;
        }
    }
    fputc('"', f); ++written;
    if (flag_dots) written += fprintf(f, "...");
    return written;
}

void i3ipc__json_token_err(I3ipc_json_token tok) {
    if (tok.type == 0) {
        fprintf(i3ipc__err, "eof");
    } else if (tok.type < 256) {
        fprintf(i3ipc__err, "'%c'", tok.type);
    } else if (tok.type == I3IPC_JSON_NULL) {
        fprintf(i3ipc__err, "'null'");
    } else if (tok.type == I3IPC_JSON_NUMBER) {
        fprintf(i3ipc__err, "number %g", tok.number);
    } else if (tok.type == I3IPC_JSON_STRING) {
        fputs("string ", i3ipc__err);
        i3ipc__printjson_string(i3ipc__err, tok.str, tok.str_size, true);
    } else if (tok.type == I3IPC_JSON_BOOL) {
        fprintf(i3ipc__err, "bool '%s'", tok.flag ? "true" : "false");
    } else {
        assert(false);
    }
}

int i3ipc__json_matchany(I3ipc_json_state* state, int type[], int type_size, I3ipc_json_token* out_tok) {
    I3ipc_json_token tok;
    if (i3ipc__json_trymatchany(state, type, type_size, &tok)) {
        if (out_tok) *out_tok = tok;
        return 0;
    }

    state->err_flag = true;
    fprintf(i3ipc__err, "Expected token type ");
    for (int i = 0; i < type_size; ++i) {
        if (i > 0) fprintf(i3ipc__err, ", ");
        if (i > 0 && i+1 == type_size) fprintf(i3ipc__err, "or ");
        if (type[i] < 256) {
            fprintf(i3ipc__err, "'%c'", type[i]);
        } else if (type[i] < I3IPC_JSON_TOKEN_MAX) {
            fprintf(i3ipc__err, "'%s'", i3ipc__global_token_type_name[type[i]-256]);
        } else {
            assert(false);
        }
    }
    fprintf(i3ipc__err, ", got ");
    i3ipc__json_token_err(tok);
    fputs("\n", i3ipc__err);

    fputs("while parsing\n    ", i3ipc__err);
    int n = 8;
    int cur = state->tokens_cur; 
    int token_first = cur - n >= 0                  ? cur - n : 0;
    int token_last  = cur + n <= state->tokens_size ? cur + n : state->tokens_size;
    size_t cur_pos = 0, cur_size = 0;
    for (int i = token_first; i < token_last; ++i) {
        I3ipc_json_token tok = state->tokens[i];
        size_t written = 0;
        if (tok.type < 256) {
            fputc(tok.type, i3ipc__err); ++written;
        } else if (tok.type == I3IPC_JSON_BOOL) {
            written += fprintf(i3ipc__err, "%s", tok.flag ? "true" : "false");
        } else if (tok.type == I3IPC_JSON_NULL) {
            written += fprintf(i3ipc__err, "null");
        } else if (tok.type == I3IPC_JSON_STRING) {
            written += i3ipc__printjson_string(i3ipc__err, tok.str, tok.str_size, true);
        } else if (tok.type == I3IPC_JSON_NUMBER) {
            if (-9223372036854775808.0 <= tok.number && tok.number < 9223372036854775808.0
                    && (double)(ssize_t)tok.number == tok.number) {
                written += fprintf(i3ipc__err, "%ld", (long)tok.number_long);
            } else {
                written += fprintf(i3ipc__err, "%.10e", tok.number);
            }
        } else {
            assert(false);
        }
        if (i < cur) {
            cur_pos += written;
        } else if (i == cur) {
            cur_size = written;
        }
    }
    fprintf(i3ipc__err, "\n%*s^", 4+(int)cur_pos, "");
    for (size_t i = 1; i < cur_size; ++i) {
        fputc('~', i3ipc__err);
    }
    fputc('\n', i3ipc__err);
    return 1;
}

bool i3ipc__json_trymatch(I3ipc_json_state* state, int type, I3ipc_json_token* out_tok) {
    int type_[] = {type};
    return i3ipc__json_trymatchany(state, type_, 1, out_tok);
}

int i3ipc__json_match(I3ipc_json_state* state, int type, I3ipc_json_token* out_tok) {
    int type_[] = {type};
    return i3ipc__json_matchany(state, type_, 1, out_tok);
}

bool i3ipc__json_looparr(I3ipc_json_state* state, int* io_iter) {
    assert(io_iter);
    if (*io_iter == 0) {
        if (i3ipc__json_match(state, '[', NULL)) goto err;
        if (i3ipc__json_trymatch(state, ']', NULL)) return false;
    } else {
        I3ipc_json_token tok;
        int type[] = {',', ']'};
        if (i3ipc__json_matchany(state, type, 2, &tok)) goto err;
        if (tok.type != ',') return false;
    }
    return true;

  err:
    state->err_flag = true;
    return false;
}

bool i3ipc__json_loopobj(I3ipc_json_state* state, int* io_iter, I3ipc_json_token* out_key) {
    assert(io_iter);
    if (*io_iter == 0) {
        if (i3ipc__json_match(state, '{', NULL)) goto err;
        if (i3ipc__json_trymatch(state, '}', NULL)) return false;
    } else {
        I3ipc_json_token tok;
        int type[] = {',', '}'};
        if (i3ipc__json_matchany(state, type, 2, &tok)) goto err;
        if (tok.type != ',') return false;
    }

    I3ipc_json_token key;
    if (i3ipc__json_match(state, I3IPC_JSON_STRING, &key)) return false;
    if (i3ipc__json_match(state, ':', NULL)) return false;
    if (out_key) *out_key = key;
    return true;

  err:
    state->err_flag = true;
    return false;
}

int i3ipc__json_skip(I3ipc_json_state* state) {
    I3ipc_json_token tok;
    {int type[] = {'{', '[', I3IPC_JSON_STRING, I3IPC_JSON_NUMBER, I3IPC_JSON_BOOL, I3IPC_JSON_NULL};
    int type_size = sizeof(type) / sizeof(type[0]);    
    if (i3ipc__json_matchany(state, type, type_size, &tok)) return 1;}
    
    if (tok.type == '{') {
        --state->tokens_cur;
        for (int i = 0; i3ipc__json_loopobj(state, &i, NULL); ++i) {
            if (i3ipc__json_skip(state)) return 4;
        }
        if (state->err_flag) return 9;
    } else if (tok.type == '[') {
        --state->tokens_cur;
        for (int i = 0; i3ipc__json_looparr(state, &i); ++i) {
            if (i3ipc__json_skip(state)) return 4;
        }
        if (state->err_flag) return 9;
    } else {
        /* nothing, the code above ensures that we only get a value here */
    }

    return 0;
}

int i3ipc__parse_key(I3ipc_json_state* state, I3ipc_json_token* out_key) {
    assert(state);
    I3ipc_json_token key;
    if (i3ipc__json_match(state, I3IPC_JSON_STRING, &key)) return 1;
    if (i3ipc__json_match(state, ':', NULL)) return 2;
    if (out_key) *out_key = key;
    return 0;
}

enum I3ipc_type_flags {
    I3IPC_TYPE_ISARRAY =   1,
    I3IPC_TYPE_ISSIZE  =   2,
    I3IPC_TYPE_ISENUM  =   4,
    I3IPC_TYPE_ISPTR   =   8,
    I3IPC_TYPE_ISOPT   =  16,
    I3IPC_TYPE_ISOMIT  =  32,
    I3IPC_TYPE_ISFLAG  =  64,
    
    I3IPC_TYPE_MARKER  = 128,
    I3IPC_TYPE_INLINE  = 256,

    I3IPC_TYPE_GROUP_DERIVED = I3IPC_TYPE_ISSIZE | I3IPC_TYPE_ISENUM | I3IPC_TYPE_ISFLAG,
    I3IPC_TYPE_GROUP_MAYBE   = I3IPC_TYPE_ISPTR | I3IPC_TYPE_ISOPT
};

typedef struct I3ipc_field {
    int type;
    int flags;
    char const* name;
    int offset;
    char const* full_name;
    char const* json_name;
} I3ipc_field;

typedef struct I3ipc_type {
    int type;
    bool is_primitive;
    bool is_inline;
    size_t size;
    size_t alignment;
    I3ipc_field* fields;
    int fields_size;
    char const* name;
} I3ipc_type;

#define I3IPC__TYPE_BEGIN2(type, T, flags) {(type), I3IPC_TYPE_MARKER | flags, NULL, sizeof(T), (char*)I3IPC_ALIGNOF(T), #T}
#define I3IPC__TYPE_BEGIN(type, T) I3IPC__TYPE_BEGIN2(type, T, 0)
#define I3IPC__DOFIELD2(T, subtype, name, flags) {(subtype), (flags), #name, offsetof(T, name), #T "." #name, NULL}
#define I3IPC__DOFIELD(T, subtype, name) I3IPC__DOFIELD2(T, subtype, name, 0) 
#define I3IPC__DOPTR(T, subtype, name) I3IPC__DOFIELD2(T, subtype, name, I3IPC_TYPE_ISPTR) 
#define I3IPC__DOMAYBE(T, subtype, name) \
    I3IPC__DOFIELD2(T, subtype, name, I3IPC_TYPE_ISOPT), \
    I3IPC__DOFIELD2(T, I3IPC_TYPE_BOOL, name##_set, I3IPC_TYPE_ISFLAG) 
#define I3IPC__DOARRAY2(T, subtype, name, flags) \
    I3IPC__DOFIELD2(T, subtype, name, I3IPC_TYPE_ISARRAY | flags), \
    I3IPC__DOFIELD2(T, I3IPC_TYPE_INT, name##_size, I3IPC_TYPE_ISSIZE) 
#define I3IPC__DOARRAY3(T, subtype, name) I3IPC__DOARRAY2(T, subtype, name, I3IPC_TYPE_ISOPT | I3IPC_TYPE_ISOMIT)
#define I3IPC__DOENUM2(T, subtype, name, flags) \
    I3IPC__DOARRAY2(T, subtype, name, flags), \
    I3IPC__DOFIELD2(T, I3IPC_TYPE_INT, name##_enum, I3IPC_TYPE_ISENUM) 
#define I3IPC__DOARRAY(T, subtype, name) I3IPC__DOARRAY2(T, subtype, name, 0) 
#define I3IPC__DOENUM(T, subtype, name) I3IPC__DOENUM2(T, subtype, name, 0)

static I3ipc_field i3ipc__global_fields[] = {
    I3IPC__TYPE_BEGIN(I3IPC_TYPE_BOOL, bool),
    I3IPC__TYPE_BEGIN(I3IPC_TYPE_CHAR, char),
    I3IPC__TYPE_BEGIN(I3IPC_TYPE_INT, int),
    I3IPC__TYPE_BEGIN(I3IPC_TYPE_FLOAT, float),
    I3IPC__TYPE_BEGIN(I3IPC_TYPE_SIZET, size_t),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_RECT, I3ipc_rect),
    I3IPC__DOFIELD(I3ipc_rect, I3IPC_TYPE_INT, x),
    I3IPC__DOFIELD(I3ipc_rect, I3IPC_TYPE_INT, y),
    I3IPC__DOFIELD(I3ipc_rect, I3IPC_TYPE_INT, width),
    I3IPC__DOFIELD(I3ipc_rect, I3IPC_TYPE_INT, height),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_STRING, I3ipc_string),
    I3IPC__DOARRAY(I3ipc_string, I3IPC_TYPE_CHAR, str),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_NODE_WINDOW_PROPERTIES, I3ipc_node_window_properties),
    I3IPC__DOARRAY2(I3ipc_node_window_properties, I3IPC_TYPE_CHAR, title, I3IPC_TYPE_ISOPT),
    I3IPC__DOARRAY2(I3ipc_node_window_properties, I3IPC_TYPE_CHAR, instance, I3IPC_TYPE_ISOPT),
    I3IPC__DOARRAY2(I3ipc_node_window_properties, I3IPC_TYPE_CHAR, window_class, I3IPC_TYPE_ISOPT),
    I3IPC__DOARRAY2(I3ipc_node_window_properties, I3IPC_TYPE_CHAR, window_role, I3IPC_TYPE_ISOPT),
    I3IPC__DOMAYBE (I3ipc_node_window_properties, I3IPC_TYPE_INT, transient_for),
    
    I3IPC__TYPE_BEGIN(I3IPC_TYPE_NODE, I3ipc_node),
    I3IPC__DOFIELD(I3ipc_node, I3IPC_TYPE_SIZET, id),
    I3IPC__DOARRAY2(I3ipc_node, I3IPC_TYPE_CHAR, name, I3IPC_TYPE_ISOPT),
    I3IPC__DOENUM (I3ipc_node, I3IPC_TYPE_CHAR, type),
    I3IPC__DOENUM (I3ipc_node, I3IPC_TYPE_CHAR, border),
    I3IPC__DOFIELD(I3ipc_node, I3IPC_TYPE_INT, current_border_width),
    I3IPC__DOENUM (I3ipc_node, I3IPC_TYPE_CHAR, layout),
    I3IPC__DOENUM (I3ipc_node, I3IPC_TYPE_CHAR, orientation),
    I3IPC__DOMAYBE(I3ipc_node, I3IPC_TYPE_FLOAT, percent),
    I3IPC__DOFIELD(I3ipc_node, I3IPC_TYPE_RECT, rect),
    I3IPC__DOFIELD(I3ipc_node, I3IPC_TYPE_RECT, window_rect),
    I3IPC__DOFIELD(I3ipc_node, I3IPC_TYPE_RECT, deco_rect),
    I3IPC__DOFIELD(I3ipc_node, I3IPC_TYPE_RECT, geometry),
    I3IPC__DOMAYBE(I3ipc_node, I3IPC_TYPE_INT, window),
    I3IPC__DOPTR  (I3ipc_node, I3IPC_TYPE_NODE_WINDOW_PROPERTIES, window_properties),
    I3IPC__DOENUM2(I3ipc_node, I3IPC_TYPE_CHAR, window_type, I3IPC_TYPE_ISOPT),
    I3IPC__DOFIELD(I3ipc_node, I3IPC_TYPE_BOOL, urgent),
    I3IPC__DOARRAY3(I3ipc_node, I3IPC_TYPE_STRING, marks),
    I3IPC__DOFIELD(I3ipc_node, I3IPC_TYPE_BOOL, focused),
    I3IPC__DOARRAY(I3ipc_node, I3IPC_TYPE_SIZET, focus),
    I3IPC__DOFIELD(I3ipc_node, I3IPC_TYPE_INT, fullscreen_mode),
    I3IPC__DOARRAY(I3ipc_node, I3IPC_TYPE_NODE, nodes),
    I3IPC__DOARRAY(I3ipc_node, I3IPC_TYPE_NODE, floating_nodes),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_BAR_CONFIG_COLORS, I3ipc_bar_config_colors),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, background),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, statusline),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, separator),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, focused_background),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, focused_statusline),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, focused_separator),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, focused_workspace_text),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, focused_workspace_bg),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, focused_workspace_border),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, active_workspace_text),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, active_workspace_bg),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, active_workspace_border),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, inactive_workspace_text),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, inactive_workspace_bg),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, inactive_workspace_border),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, urgent_workspace_text),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, urgent_workspace_bg),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, urgent_workspace_border),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, binding_mode_text),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, binding_mode_bg),
    I3IPC__DOARRAY3(I3ipc_bar_config_colors, I3IPC_TYPE_CHAR, binding_mode_border),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_BAR_CONFIG, I3ipc_bar_config),
    I3IPC__DOARRAY(I3ipc_bar_config, I3IPC_TYPE_CHAR, id),
    I3IPC__DOENUM (I3ipc_bar_config, I3IPC_TYPE_CHAR, mode),
    I3IPC__DOENUM (I3ipc_bar_config, I3IPC_TYPE_CHAR, position),
    I3IPC__DOARRAY(I3ipc_bar_config, I3IPC_TYPE_CHAR, status_command),
    I3IPC__DOARRAY(I3ipc_bar_config, I3IPC_TYPE_CHAR, font),
    I3IPC__DOFIELD(I3ipc_bar_config, I3IPC_TYPE_BOOL, workspace_buttons),
    I3IPC__DOFIELD(I3ipc_bar_config, I3IPC_TYPE_BOOL, binding_mode_indicator),
    I3IPC__DOFIELD(I3ipc_bar_config, I3IPC_TYPE_BOOL, verbose),
    I3IPC__DOFIELD(I3ipc_bar_config, I3IPC_TYPE_BAR_CONFIG_COLORS, colors),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_REPLY_COMMAND_EL, I3ipc_reply_command_el),
    I3IPC__DOFIELD(I3ipc_reply_command_el, I3IPC_TYPE_BOOL, success),
    I3IPC__DOARRAY3(I3ipc_reply_command_el, I3IPC_TYPE_CHAR, error),

    I3IPC__TYPE_BEGIN2(I3IPC_TYPE_REPLY_COMMAND, I3ipc_reply_command, I3IPC_TYPE_INLINE),
    I3IPC__DOARRAY(I3ipc_reply_command, I3IPC_TYPE_REPLY_COMMAND_EL, commands),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_REPLY_WORKSPACES_EL, I3ipc_reply_workspaces_el),
    I3IPC__DOFIELD(I3ipc_reply_workspaces_el, I3IPC_TYPE_SIZET, id),
    I3IPC__DOFIELD(I3ipc_reply_workspaces_el, I3IPC_TYPE_INT, num),
    I3IPC__DOARRAY(I3ipc_reply_workspaces_el, I3IPC_TYPE_CHAR, name),
    I3IPC__DOFIELD(I3ipc_reply_workspaces_el, I3IPC_TYPE_BOOL, visible),
    I3IPC__DOFIELD(I3ipc_reply_workspaces_el, I3IPC_TYPE_BOOL, focused),
    I3IPC__DOFIELD(I3ipc_reply_workspaces_el, I3IPC_TYPE_BOOL, urgent),
    I3IPC__DOFIELD(I3ipc_reply_workspaces_el, I3IPC_TYPE_RECT, rect),
    I3IPC__DOARRAY(I3ipc_reply_workspaces_el, I3IPC_TYPE_CHAR, output),
    
    I3IPC__TYPE_BEGIN2(I3IPC_TYPE_REPLY_WORKSPACES, I3ipc_reply_workspaces, I3IPC_TYPE_INLINE),
    I3IPC__DOARRAY(I3ipc_reply_workspaces, I3IPC_TYPE_REPLY_WORKSPACES_EL, workspaces),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_REPLY_SUBSCRIBE, I3ipc_reply_subscribe),
    I3IPC__DOFIELD(I3ipc_reply_subscribe, I3IPC_TYPE_BOOL, success),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_REPLY_OUTPUTS_EL, I3ipc_reply_outputs_el),
    I3IPC__DOARRAY(I3ipc_reply_outputs_el, I3IPC_TYPE_CHAR, name),
    I3IPC__DOFIELD(I3ipc_reply_outputs_el, I3IPC_TYPE_BOOL, active),
    I3IPC__DOFIELD(I3ipc_reply_outputs_el, I3IPC_TYPE_BOOL, primary),
    I3IPC__DOARRAY2(I3ipc_reply_outputs_el, I3IPC_TYPE_CHAR, current_workspace, I3IPC_TYPE_ISOPT),
    I3IPC__DOFIELD(I3ipc_reply_outputs_el, I3IPC_TYPE_RECT, rect),

    I3IPC__TYPE_BEGIN2(I3IPC_TYPE_REPLY_OUTPUTS, I3ipc_reply_outputs, I3IPC_TYPE_INLINE),
    I3IPC__DOARRAY(I3ipc_reply_outputs, I3IPC_TYPE_REPLY_OUTPUTS_EL, outputs),

    I3IPC__TYPE_BEGIN2(I3IPC_TYPE_REPLY_TREE, I3ipc_reply_tree, I3IPC_TYPE_INLINE),
    I3IPC__DOFIELD(I3ipc_reply_tree, I3IPC_TYPE_NODE, root),

    I3IPC__TYPE_BEGIN2(I3IPC_TYPE_REPLY_MARKS, I3ipc_reply_marks, I3IPC_TYPE_INLINE),
    I3IPC__DOARRAY(I3ipc_reply_marks, I3IPC_TYPE_STRING, marks),

    I3IPC__TYPE_BEGIN2(I3IPC_TYPE_REPLY_BAR_CONFIG_IDS, I3ipc_reply_bar_config_ids, I3IPC_TYPE_INLINE),
    I3IPC__DOARRAY(I3ipc_reply_bar_config_ids, I3IPC_TYPE_STRING, ids),

    I3IPC__TYPE_BEGIN2(I3IPC_TYPE_REPLY_BAR_CONFIG, I3ipc_reply_bar_config, I3IPC_TYPE_INLINE),
    I3IPC__DOFIELD(I3ipc_reply_bar_config, I3IPC_TYPE_BAR_CONFIG, cfg),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_REPLY_VERSION, I3ipc_reply_version),
    I3IPC__DOFIELD(I3ipc_reply_version, I3IPC_TYPE_INT, major),
    I3IPC__DOFIELD(I3ipc_reply_version, I3IPC_TYPE_INT, minor),
    I3IPC__DOFIELD(I3ipc_reply_version, I3IPC_TYPE_INT, patch),
    I3IPC__DOARRAY(I3ipc_reply_version, I3IPC_TYPE_CHAR, human_readable),
    I3IPC__DOARRAY(I3ipc_reply_version, I3IPC_TYPE_CHAR, loaded_config_file_name),

    I3IPC__TYPE_BEGIN2(I3IPC_TYPE_REPLY_BINDING_MODES, I3ipc_reply_binding_modes, I3IPC_TYPE_INLINE),
    I3IPC__DOARRAY(I3ipc_reply_binding_modes, I3IPC_TYPE_STRING, modes),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_REPLY_CONFIG, I3ipc_reply_config),
    I3IPC__DOARRAY(I3ipc_reply_config, I3IPC_TYPE_CHAR, config),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_REPLY_TICK, I3ipc_reply_tick),
    I3IPC__DOFIELD(I3ipc_reply_tick, I3IPC_TYPE_BOOL, success),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_REPLY_SYNC, I3ipc_reply_sync),
    I3IPC__DOFIELD(I3ipc_reply_sync, I3IPC_TYPE_BOOL, success),

    /* This type is mostly a dummy. */
    I3IPC__TYPE_BEGIN(I3IPC_TYPE_EVENT, I3ipc_event),
    
    I3IPC__TYPE_BEGIN(I3IPC_TYPE_EVENT_WORKSPACE, I3ipc_event_workspace),
    I3IPC__DOENUM (I3ipc_event_workspace, I3IPC_TYPE_CHAR, change),
    I3IPC__DOPTR  (I3ipc_event_workspace, I3IPC_TYPE_NODE, current),
    I3IPC__DOPTR  (I3ipc_event_workspace, I3IPC_TYPE_NODE, old),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_EVENT_OUTPUT, I3ipc_event_output),
    I3IPC__DOENUM (I3ipc_event_output, I3IPC_TYPE_CHAR, change),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_EVENT_MODE, I3ipc_event_mode),
    I3IPC__DOARRAY(I3ipc_event_mode, I3IPC_TYPE_CHAR, change),
    I3IPC__DOFIELD(I3ipc_event_mode, I3IPC_TYPE_BOOL, pango_markup),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_EVENT_WINDOW, I3ipc_event_window),
    I3IPC__DOENUM (I3ipc_event_window, I3IPC_TYPE_CHAR, change),
    I3IPC__DOFIELD(I3ipc_event_window, I3IPC_TYPE_NODE, container),

    I3IPC__TYPE_BEGIN2(I3IPC_TYPE_EVENT_BARCONFIG_UPDATE, I3ipc_event_barconfig_update, I3IPC_TYPE_INLINE),
    I3IPC__DOFIELD(I3ipc_event_barconfig_update, I3IPC_TYPE_BAR_CONFIG, cfg),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_EVENT_BINDING_BINDING, I3ipc_event_binding_binding),
    I3IPC__DOARRAY (I3ipc_event_binding_binding, I3IPC_TYPE_CHAR, command),
    I3IPC__DOARRAY (I3ipc_event_binding_binding, I3IPC_TYPE_STRING, event_state_mask),
    I3IPC__DOFIELD (I3ipc_event_binding_binding, I3IPC_TYPE_INT, input_code),
    I3IPC__DOARRAY2(I3ipc_event_binding_binding, I3IPC_TYPE_CHAR, symbol, I3IPC_TYPE_ISOPT),
    I3IPC__DOENUM  (I3ipc_event_binding_binding, I3IPC_TYPE_CHAR, input_type),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_EVENT_BINDING, I3ipc_event_binding),
    I3IPC__DOENUM (I3ipc_event_binding, I3IPC_TYPE_CHAR, change),
    I3IPC__DOFIELD(I3ipc_event_binding, I3IPC_TYPE_EVENT_BINDING_BINDING, binding),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_EVENT_SHUTDOWN, I3ipc_event_shutdown),
    I3IPC__DOENUM (I3ipc_event_shutdown, I3IPC_TYPE_CHAR, change),

    I3IPC__TYPE_BEGIN(I3IPC_TYPE_EVENT_TICK, I3ipc_event_tick),
    I3IPC__DOFIELD(I3ipc_event_tick, I3IPC_TYPE_BOOL, first),
    I3IPC__DOARRAY(I3ipc_event_tick, I3IPC_TYPE_CHAR, payload),

    I3IPC__TYPE_BEGIN(-1, char)
};

#undef I3IPC__TYPE_BEGIN2
#undef I3IPC__TYPE_BEGIN
#undef I3IPC__DOFIELD2
#undef I3IPC__DOFIELD
#undef I3IPC__DOPTR
#undef I3IPC__DOMAYBE
#undef I3IPC__DOARRAY2
#undef I3IPC__DOARRAY3
#undef I3IPC__DOENUM2
#undef I3IPC__DOARRAY
#undef I3IPC__DOENUM

static I3ipc_type i3ipc__global_types[I3IPC_TYPE_COUNT];

static char const* i3ipc__global_enums[] = {
    /* There have to be in the same order as their respective enums */
    /* @Cleanup put this notice on the enums also */
    "$I3ipc_node.type", "root", "output", "con", "floating_con", "workspace", "dockarea",
    "$I3ipc_node.border", "normal", "none", "pixel",
    "$I3ipc_node.layout", "splith", "splitv", "stacked", "tabbed", "dockarea", "output",
    "$I3ipc_node.orientation", "none", "horizontal", "vertical",
    "$I3ipc_node.window_type", NULL, "normal", "dialog", "utility", "toolbar", "splash",
        "menu", "dropdown_menu", "popup_menu", "tooltip", "notification", "dock", "unknown",
    "$I3ipc_bar_config.mode", "dock", "hide",
    "$I3ipc_bar_config.position", "bottom", "top",
    "$I3ipc_event_workspace.change", "focus", "init", "empty", "urgent", "reload", "rename",
        "restored", "move",
    "$I3ipc_event_output.change", "unspecified",
    "$I3ipc_event_window.change", "new", "close", "focus", "title", "fullscreen_mode", "move",
        "floating", "urgent", "mark",
    "$I3ipc_event_binding.change", "run",
    "$I3ipc_event_binding.input_type", "keyboard", "mouse",
    "$I3ipc_event_shutdown.change", "restart", "exit",
    "$$"
};

void i3ipc__init_globals(void) {
    i3ipc__err = open_memstream(&i3ipc__error_buf, &i3ipc__error_buf_size);

    /* Set it to an invalid value, to catch errors */
    for (int i = 0; i < I3IPC_TYPE_COUNT; ++i) {
        i3ipc__global_types[i].type = -1;
    }

    {int size = sizeof(i3ipc__global_fields) / sizeof(i3ipc__global_fields[0]);
    int type = -1;
    int type_first = 0;
    for (int i = 0; i < size; ++i) {
        I3ipc_field* f = &i3ipc__global_fields[i];
        
        if (f->flags & I3IPC_TYPE_MARKER) {
            if (type != -1) {
                i3ipc__global_types[type].fields_size = i - type_first;
            }
            if (f->type != -1) {
                I3ipc_type* t = &i3ipc__global_types[f->type];
                t->type = f->type;
                t->size = f->offset;
                t->fields = &i3ipc__global_fields[i+1];
                t->is_primitive = f->type < I3IPC_TYPE_PRIMITIVE_COUNT;
                t->is_inline = f->flags & I3IPC_TYPE_INLINE;
                t->alignment = (size_t)f->full_name;
                t->name = f->json_name;
            }
            type = f->type;
            type_first = i+1;
        } else {
            /* This one field has a different name, fix it up here. */
            if (strcmp(f->full_name, "I3ipc_node_window_properties.window_class") == 0) {
                f->json_name = "class";
            } else {
                f->json_name = f->name;
            }
        }
    }}

    i3ipc__globals_initialized = true;
}

I3ipc_type i3ipc__type_get(int type) {
    assert(0 <= type && type <= I3IPC_TYPE_COUNT);
    assert(i3ipc__global_types[type].type == type);
    return i3ipc__global_types[type];
}

bool i3ipc__parse_loopfields(I3ipc_json_state* state, I3ipc_type* type, int* io_field) {
    assert(io_field);
    int field = -1;
    int j = *io_field == -1 ? 0 : 1;

    while (true) {
        I3ipc_json_token key;
        if (!i3ipc__json_loopobj(state, &j, &key)) return false;
        j = 1;
    
        for (int i = 0; i < type->fields_size; ++i) {
            if (type->fields[i].flags & I3IPC_TYPE_GROUP_DERIVED) continue;
            if (strcmp(type->fields[i].json_name, key.str) == 0) {
                field = i;
                break;
            }
        }

        if (field == -1) {
            if (i3ipc__json_skip(state)) return false;
        } else {
            break;
        }
    }
    
    *io_field = field;
    return true;
}

typedef struct I3ipc_parse_state_allocs {
        size_t size, alignment;
} I3ipc_parse_state_allocs;

typedef struct I3ipc_parse_state {
    I3ipc_context* context;
    I3ipc_json_state state;
    I3ipc_parse_state_allocs* allocs;
    int allocs_size;

    char* memory;
    bool copy_strings;
    
    int next_size;
    char* next_enum;
    bool next_set;
} I3ipc_parse_state;

void i3ipc__parse_doderived(I3ipc_parse_state* p, I3ipc_type* type, int field, char* base) {
    I3ipc_field f0 = type->fields[field];
    
    for (int i = field+1; i < type->fields_size; ++i) {
        I3ipc_field f = type->fields[i];
        if (!(f.flags & I3IPC_TYPE_GROUP_DERIVED)) break;
        
        if (f.flags & I3IPC_TYPE_ISSIZE) {
            assert(f.type == I3IPC_TYPE_INT);
            *(int*)(base + f.offset) = p->next_size;
            
        } else if (f.flags & I3IPC_TYPE_ISFLAG) {
            assert(f.type == I3IPC_TYPE_BOOL);
            *(bool*)(base + f.offset) = p->next_set;
            
        } else if (f.flags & I3IPC_TYPE_ISENUM) {
            assert(f.type == I3IPC_TYPE_INT);
            int size = sizeof(i3ipc__global_enums) / sizeof(i3ipc__global_enums[0]);
            int start = -1;
            for (int j = 0; j < size; ++j) {
                char const* s = i3ipc__global_enums[j];
                if (s && s[0] == '$' && strcmp(s+1, f0.full_name) == 0) {
                    start = j+1;
                    break;
                }
            }
            if (start == -1) continue;
            int val_enum = -1;
            for (int j = start;; ++j) {
                char const* s = i3ipc__global_enums[j];
                if (s && s[0] == '$') break;
                if (s && p->next_enum ? strcmp(s, p->next_enum) == 0 : s == p->next_enum) {
                    val_enum = j - start;
                    break;
                }
            }
            *(int*)(base + f.offset) = val_enum;
        } else {
            assert(false);
        }
    } 
}

char* i3ipc__parse_alloc(I3ipc_parse_state* p, size_t size, size_t alignment, int* out_handle) {
    if (out_handle) *out_handle = p->allocs_size;
    if (!p->memory) {
        size_t next_size = (p->allocs_size + 1) * sizeof(p->allocs[0]);
        i3ipc__context_reserve(p->context, I3IPC_CONTEXT_ALLOCS, next_size, (void**)&p->allocs);
        p->allocs[p->allocs_size].size = size;
        p->allocs[p->allocs_size].alignment = alignment;
        ++p->allocs_size;
        return NULL;
    } else {
        return p->memory + p->allocs[p->allocs_size++].size;
    }
}


int i3ipc__parse_helper(I3ipc_parse_state* p, int type_id, int type_flags, char* base) {
    assert(p);
    assert(type_id != I3IPC_TYPE_EVENT);

    bool is_string_type = (type_id == I3IPC_TYPE_STRING && type_flags == 0)
        || (type_id == I3IPC_TYPE_CHAR && (type_flags & I3IPC_TYPE_ISARRAY));
    bool was_set = true;
    char* was_enum = NULL;
    
    I3ipc_type type = i3ipc__type_get(type_id);
    
    if (type_flags & I3IPC_TYPE_GROUP_MAYBE) {
        if (!i3ipc__json_trymatch(&p->state, I3IPC_JSON_NULL, NULL)) {
            char* ptr_base = base;
            if (type_flags & I3IPC_TYPE_ISPTR) {
                ptr_base = i3ipc__parse_alloc(p, type.size, type.alignment, NULL);
                if (base) *(char**)base = ptr_base;
            }
            
            if (i3ipc__parse_helper(p, type_id, type_flags & ~I3IPC_TYPE_GROUP_MAYBE, ptr_base)) return 6;
            was_enum = p->next_enum; /* need to preserve the enum value */
        } else {
            p->next_size = 0;
            was_set = false;
            was_enum = NULL;
        }
    } else if (is_string_type) {
        I3ipc_json_token tok;
        if (i3ipc__json_match(&p->state, I3IPC_JSON_STRING, &tok)) return 1;

        char* str_base;
        if (p->copy_strings) {
            str_base = i3ipc__parse_alloc(p, tok.str_size + 1, 1, NULL);
            if (base) {
                memcpy(str_base, tok.str, tok.str_size + 1);
            }
        } else {
            str_base = tok.str;
        }
        
        if (base) {
            if (type_id == I3IPC_TYPE_STRING) {
                *(char**)(base + type.fields[0].offset) = str_base;
                *(int*)  (base + type.fields[1].offset) = tok.str_size;
            } else {
                *(char**)base = str_base;
                p->next_size = tok.str_size;
            }
            was_enum = str_base;
        }
    } else if (type_flags & I3IPC_TYPE_ISARRAY) {
        int index;
        char* arr_base = i3ipc__parse_alloc(p, -1, type.alignment, &index);
        if (base) *(char**)base = arr_base;

        int i;
        for (i = 0; i3ipc__json_looparr(&p->state, &i); ++i) {
            char* i_arr_base = base ? arr_base + i * type.size : NULL;
            if (i3ipc__parse_helper(p, type_id, 0, i_arr_base)) return 6;
        }
        if (p->state.err_flag) return 9;

        if (!base) p->allocs[index].size = i * type.size;
        p->next_size = i;
    } else if (type.is_inline) {
        assert(type.fields_size > 0);
        for (int i = 1; i < type.fields_size; ++i) {
            assert(type.fields[i].flags & I3IPC_TYPE_GROUP_DERIVED);
        }

        I3ipc_field field = type.fields[0];
        char* inline_base = base ? base + field.offset : NULL;
        if (i3ipc__parse_helper(p, field.type, field.flags, inline_base)) return 17;
        if (base) i3ipc__parse_doderived(p, &type, 0, base);
    } else if (type_flags == 0 && type.is_primitive) {
        int tok_type = type_id == I3IPC_TYPE_BOOL ? I3IPC_JSON_BOOL : I3IPC_JSON_NUMBER;
        I3ipc_json_token tok;
        if (i3ipc__json_match(&p->state, tok_type, &tok)) return 1;
        switch (base ? type_id : -1) {
        case -1: break;
        case I3IPC_TYPE_BOOL:  *(bool*)base = tok.flag; break;
        case I3IPC_TYPE_SIZET: *(size_t*)base = tok.number_long; break;
        case I3IPC_TYPE_INT:   *(int*)base = (int)tok.number_long; break;
        case I3IPC_TYPE_FLOAT: *(float*)base = (float)tok.number; break;
        /* case I3IPC_TYPE_CHAR:  *base = (char)tok.number_long; break; */
        default: assert(false);
        }
    } else if (type_flags == 0) {
        for (int field = -1; i3ipc__parse_loopfields(&p->state, &type, &field);) {
            char* field_base = base ? base + type.fields[field].offset : NULL;
            if (i3ipc__parse_helper(
                p, type.fields[field].type, type.fields[field].flags, field_base
            )) return 3;

            if (base) i3ipc__parse_doderived(p, &type, field, base);
        }
        if (p->state.err_flag) return 9;
    } else {
        assert(false);
    }

    p->next_set = was_set;
    p->next_enum = was_enum;
    return 0;
}

int i3ipc_parse_try(I3ipc_message* msg, int message_type, int type_id, char** out_data) {
    assert(msg);
    if (i3ipc_error_code()) return I3IPC_ERROR_BADSTATE;
    {int code = i3ipc_init_try(NULL);
    if (code) return code;}

    I3ipc_context* context = &i3ipc__global_context;

    /* Initialise parse state */
    I3ipc_parse_state p;
    memset(&p, 0, sizeof(p));
    p.context = context;
    p.allocs = (I3ipc_parse_state_allocs*)context->buffers[I3IPC_CONTEXT_ALLOCS];
    p.copy_strings = !context->staticalloc;

    if (msg->message_type != message_type) {
        fprintf(i3ipc__err, "Unexpected reply type, expected %s(%x), got %s(%x)\n",
            i3ipc__message_type_str(message_type, true), message_type,
            i3ipc__message_type_str(msg->message_type, true), msg->message_type);
        return i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
    }
    
    memset(&p.state, 0, sizeof(p.state));
    p.state.cur = (char*)(msg + 1);
    p.state.left = msg->message_length;
    p.state.tokens = (I3ipc_json_token*)context->buffers[I3IPC_CONTEXT_JSON];
    if (i3ipc__json_scan(context, &p.state)) {
        return i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
    }

    msg->message_length = 0; /* Safety precaution, as we will change the contents */

    /* Allocate space for the base type */
    I3ipc_type type = i3ipc__type_get(type_id);
    i3ipc__parse_alloc(&p, type.size, type.alignment, NULL);

    /* First pass, determine sizes of things */
    if (i3ipc__parse_helper(&p, type_id, 0, NULL)) {
        return i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
    }
    if (i3ipc__json_match(&p.state, 0, NULL)) {
        return i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
    }

    /* Process allocations, fixing alignment */
    size_t off = 0;
    for (int i = 0; i < p.allocs_size; ++i) {
        size_t i_size = p.allocs[i].size;
        off = (off + p.allocs[i].alignment-1) & ~(p.allocs[i].alignment-1);
        p.allocs[i].size = off;
        off += i_size;
    }
    size_t total_size = off;

    /* Allocate memory */
    if (context->staticalloc) {
        i3ipc__context_reserve(context, I3IPC_CONTEXT_PARSE, total_size, (void**)&p.memory);
        memset(p.memory, 0, total_size);
    } else {
        p.memory = (char*)calloc(total_size, 1);
    }
    p.allocs_size = 0;
    p.state.tokens_cur = 0;
    char* base = i3ipc__parse_alloc(&p, type.size, type.alignment, NULL);

    /* Second pass, actually parse */
    if (i3ipc__parse_helper(&p, type_id, 0, base)) {
        return i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
    }

    if (out_data) *out_data = p.memory;
    return 0;
}

void i3ipc__type_readderived(I3ipc_type* type, int field, char* base, bool* out_set, int* out_size, int* out_enum) {
    bool field_set = true;
    int field_size = -1;
    int field_enum = -1;
    for (int j = field + 1; j < type->fields_size; ++j) {
        I3ipc_field j_field = type->fields[j];
        if (!(j_field.flags & I3IPC_TYPE_GROUP_DERIVED)) break;
                
        if (j_field.flags & I3IPC_TYPE_ISFLAG) {
            assert(j_field.type == I3IPC_TYPE_BOOL);
            field_set = *(bool*)(base + j_field.offset);
        } else if (j_field.flags & I3IPC_TYPE_ISSIZE) {
            assert(j_field.type == I3IPC_TYPE_INT);
            field_size = *(int*)(base + j_field.offset);
        } else if (j_field.flags & I3IPC_TYPE_ISENUM) {
            assert(j_field.type == I3IPC_TYPE_INT);
            field_enum = *(int*)(base + j_field.offset);
        } else {
            assert(false);
        }
    }

    if (out_set) *out_set = field_set;
    if (out_size) *out_size = field_size;
    if (out_enum) *out_enum = field_enum;
}

bool i3ipc__field_is_set(int type_id, int type_flags, char* base) {
    bool is_string_type = (type_id == I3IPC_TYPE_STRING && type_flags == 0)
        || (type_id == I3IPC_TYPE_CHAR && (type_flags & I3IPC_TYPE_ISARRAY));
    
    I3ipc_type type = i3ipc__type_get(type_id);
    if (type_flags & (I3IPC_TYPE_ISPTR | I3IPC_TYPE_ISOPT)) {
        return *(char**)base;
    } else if (is_string_type) {
        return type_id == I3IPC_TYPE_STRING
            ? *(char**)(base + type.fields[0].offset)
            : *(char**)base;
    } else if (type_flags & I3IPC_TYPE_ISARRAY) {
        return *(char**)base;
    } else if (type.is_inline) {
        I3ipc_field field = type.fields[0];
        return i3ipc__field_is_set(field.type, field.flags, base + field.offset);
    } else if (type_flags == 0 && type.is_primitive) {
        return true;
    } else if (type_flags == 0) {
        return true;
    } else {
        assert(false);
    }
}

int i3ipc__message_type_to_event(int message_type) {
    switch (message_type) {
    case I3IPC_EVENT_WORKSPACE:        return I3IPC_TYPE_EVENT_WORKSPACE;
    case I3IPC_EVENT_OUTPUT:           return I3IPC_TYPE_EVENT_OUTPUT;
    case I3IPC_EVENT_MODE:             return I3IPC_TYPE_EVENT_MODE;
    case I3IPC_EVENT_WINDOW:           return I3IPC_TYPE_EVENT_WINDOW;
    case I3IPC_EVENT_BARCONFIG_UPDATE: return I3IPC_TYPE_EVENT_BARCONFIG_UPDATE;
    case I3IPC_EVENT_BINDING:          return I3IPC_TYPE_EVENT_BINDING;
    case I3IPC_EVENT_SHUTDOWN:         return I3IPC_TYPE_EVENT_SHUTDOWN;
    case I3IPC_EVENT_TICK:             return I3IPC_TYPE_EVENT_TICK;
    default: return -1;
    }
}

void i3ipc__printjson_helper(FILE* f, int type_id, int type_flags, char* base, int size) {
    bool is_string_type = (type_id == I3IPC_TYPE_STRING && type_flags == 0)
        || (type_id == I3IPC_TYPE_CHAR && (type_flags & I3IPC_TYPE_ISARRAY));
    
    I3ipc_type type = i3ipc__type_get(type_id);
    if (type_flags & I3IPC_TYPE_ISPTR) {
        char* ptr_base = *(char**)base;
        assert(ptr_base);
        i3ipc__printjson_helper(f, type_id, type_flags ^ I3IPC_TYPE_ISPTR, ptr_base, -1);
    } else if (is_string_type) {
        char* str; int str_size;
        if (type_id == I3IPC_TYPE_STRING) {
            str      = *(char**)(base + type.fields[0].offset);
            str_size = *(int*)  (base + type.fields[1].offset);
        } else {
            assert(size != -1);
            str = *(char**)base;
            str_size = size;
        }
        assert(str);
        i3ipc__printjson_string(f, str, str_size, false);
    } else if (type_flags & I3IPC_TYPE_ISARRAY) {
        assert(size != -1);
        char* arr_base = *(char**)base;
        assert(arr_base);
        
        fputc('[', f);
        for (int i = 0; i < size; ++i) {
            if (i) fputc(',', f);
            i3ipc__printjson_helper(f, type_id, 0, arr_base + i*type.size, -1);
        }
        fputc(']', f);
    } else if (type.is_inline) {
        assert(type.fields_size > 0);
        int size = -1;
        for (int i = 1; i < type.fields_size; ++i) {
            I3ipc_field i_field = type.fields[i];
            assert(i_field.flags & I3IPC_TYPE_GROUP_DERIVED);
            if (i_field.flags & I3IPC_TYPE_ISSIZE) {
                assert(i_field.type == I3IPC_TYPE_INT);
                size = *(int*)(base + i_field.offset);
            } else {
                assert(false);
            }
        }

        I3ipc_field field = type.fields[0];
        i3ipc__printjson_helper(f, field.type, field.flags, base + field.offset, size);
    } else if (type_flags == 0 && type.is_primitive) {
        switch (type_id) {
        case I3IPC_TYPE_BOOL: fputs(*(bool*)base ? "true" : "false", f); break;
        case I3IPC_TYPE_SIZET: fprintf(f, "%lu", (long)*(size_t*)base); break;
        case I3IPC_TYPE_INT: fprintf(f, "%d", *(int*)base); break;
        case I3IPC_TYPE_FLOAT: fprintf(f, "%f", *(float*)base); break;
        default: assert(false);
        }
    } else if (type_flags == 0 && type_id == I3IPC_TYPE_EVENT) {
        I3ipc_event* ev = (I3ipc_event*)base;
        int subtype = i3ipc__message_type_to_event(ev->type);
        i3ipc__printjson_helper(f, subtype, type_flags, base, size);
    } else if (type_flags == 0) {
        fputc('{', f);
        for (int i = 0; i < type.fields_size; ++i) {
            I3ipc_field field = type.fields[i];
            if (field.flags & I3IPC_TYPE_GROUP_DERIVED) continue;
                        
            bool i_set = true;
            int i_size = -1;
            i3ipc__type_readderived(&type, i, base, &i_set, &i_size, NULL);

            i_set = i_set && i3ipc__field_is_set(field.type, field.flags, base + field.offset);

            if (!i_set && (field.flags & I3IPC_TYPE_ISOMIT)) {
                continue; /* omit this field */
            } else if (!i_set && (field.flags & I3IPC_TYPE_ISOPT)) {
                /* fall through, print null */
            } else if (!i_set) {
                continue; /* this is malformed input, but we omit, to match the input json */
            }

            if (i) fputc(',', f);
            i3ipc__printjson_string(f, field.json_name, -1, false);
            fputc(':', f);

            if (i_set) {
                i3ipc__printjson_helper(f, field.type, field.flags & ~I3IPC_TYPE_ISOPT, base + field.offset, i_size);
            } else {
                fputs("null", f);
            }
        }
        fputc('}', f);
    } else {
        assert(false);
    }
}

void i3ipc_printjson(int type_id, void* obj, FILE* f) {
    assert(obj);
    if (i3ipc_error_code()) return;

    if (f == NULL) f = stdout;
    i3ipc__printjson_helper(f, type_id, 0, (char*)obj, -1);
}

int i3ipc_message_and_parse_try(
    int message, int type, char const* payload, int payload_size, char** out_data
) {
    assert(out_data);
    if (i3ipc_error_code()) return I3IPC_ERROR_BADSTATE;
    
    I3ipc_message* msg;
    {int code = i3ipc_message_try(message, payload, payload_size, &msg);
    if (code) return code;}

    {int code = i3ipc_parse_try(msg, message, type, out_data);
    if (code) return code;}

    return 0;
}


I3ipc_reply_command* i3ipc_run_command(char const* commands) {
    I3ipc_reply_command* reply = NULL;
    i3ipc_message_and_parse_try(I3IPC_RUN_COMMAND, I3IPC_TYPE_REPLY_COMMAND, (char*)commands, -1, (char**)&reply);
    return reply;
}
void i3ipc_run_command_simple(char const* command) {
    bool prev = i3ipc_set_staticalloc(true);
    I3ipc_reply_command* reply = i3ipc_run_command(command);
    i3ipc_set_staticalloc(prev);
    if (!reply) return;

    for (int i = 0; i < reply->commands_size; ++i) {
        if (!reply->commands[i].success) {
            fprintf(i3ipc__err, "run command failed\n");
            fprintf(i3ipc__err, "with error: '%s'\n", reply->commands[i].error);
            fprintf(i3ipc__err, "while executing subcommand %d of command: '%s'\n", i, command);
            i3ipc__error_handle(I3IPC_ERROR_FAILED);
        }
    }
}

I3ipc_reply_workspaces* i3ipc_get_workspaces(void) {
    I3ipc_reply_workspaces* reply = NULL;
    i3ipc_message_and_parse_try(I3IPC_GET_WORKSPACES, I3IPC_TYPE_REPLY_WORKSPACES, NULL, 0, (char**)&reply);
    return reply;
}

void i3ipc_subscribe(int* event_type, int event_type_size) {
    assert(event_type || !event_type_size);
    I3ipc_context* context = &i3ipc__global_context;
    
    /* Could use the json output here, but that is much too complicated for such a simple task */

    size_t size = 2;
    for (int i = 0; i < event_type_size; ++i) {
        int ev = event_type[i];
        assert(I3IPC_EVENT_TYPE_BEGIN <= ev && ev < I3IPC_EVENT_TYPE_END);
        char const* c = i3ipc__global_event_type_name[ev - I3IPC_EVENT_TYPE_BEGIN];
        size += (i > 0) + 2 + strlen(c);
    }

    char* buf = NULL;
    i3ipc__context_reserve(context, I3IPC_CONTEXT_PAYLOAD, size + 1, (void**)&buf);
        
    size_t pos = 0;

    /* WARNING: Adjust the size calculation above if you change this code */
    buf[pos++] = '[';
    for (int i = 0; i < event_type_size; ++i) {
        int ev = event_type[i];
        assert(I3IPC_EVENT_TYPE_BEGIN <= ev && ev < I3IPC_EVENT_TYPE_END);
        if (i) buf[pos++] = ',';
        buf[pos++] = '"';
        char const* c = i3ipc__global_event_type_name[ev - I3IPC_EVENT_TYPE_BEGIN];
        for (int j = 0; c[j] && j < 256; ++j) {
            buf[pos++] = c[j];
        }
        buf[pos++] = '"';
    }
    buf[pos++] = ']';
    buf[pos] = 0;

    assert(pos == size);

    {int code = i3ipc_message_send_try(I3IPC_SUBSCRIBE, buf, size);
    if (code) return;}
    
    I3ipc_message* msg;
    {int code = i3ipc_message_receive_reorder_try(I3IPC_REPLY_SUBSCRIBE, &msg);
    if (code) return;}

    {I3ipc_reply_subscribe* reply = NULL;
    int code = i3ipc_parse_try(msg, I3IPC_REPLY_SUBSCRIBE, I3IPC_TYPE_REPLY_SUBSCRIBE, (char**)&reply);
    if (code) return;

    if (!reply->success) {
        i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
        return;
    }}
    
    return;
}
void i3ipc_subscribe_single(int event_type) {
    int arr[1] = {event_type};
    i3ipc_subscribe(arr, 1);
}

I3ipc_reply_outputs* i3ipc_get_outputs(void) {
    I3ipc_reply_outputs* reply = NULL;
    i3ipc_message_and_parse_try(I3IPC_GET_OUTPUTS, I3IPC_TYPE_REPLY_OUTPUTS, NULL, 0, (char**)&reply);
    return reply;
}
I3ipc_reply_tree* i3ipc_get_tree(void) {
    I3ipc_reply_tree* reply = NULL;
    i3ipc_message_and_parse_try(I3IPC_GET_TREE, I3IPC_TYPE_REPLY_TREE, NULL, 0, (char**)&reply);
    return reply;
}
I3ipc_reply_marks* i3ipc_get_marks(void) {
    I3ipc_reply_marks* reply = NULL;
    i3ipc_message_and_parse_try(I3IPC_GET_MARKS, I3IPC_TYPE_REPLY_MARKS, NULL, 0, (char**)&reply);
    return reply;
}

I3ipc_reply_bar_config_ids* i3ipc_get_bar_config_ids(void) {
    I3ipc_reply_bar_config_ids* reply = NULL;
    i3ipc_message_and_parse_try(
        I3IPC_GET_BAR_CONFIG, I3IPC_TYPE_REPLY_BAR_CONFIG_IDS, NULL, 0, (char**)&reply
    );
    return reply;
}
I3ipc_reply_bar_config* i3ipc_get_bar_config(char const* name) {
    I3ipc_reply_bar_config* reply = NULL;
    i3ipc_message_and_parse_try(
        I3IPC_GET_BAR_CONFIG, I3IPC_TYPE_REPLY_BAR_CONFIG, (char*)name, -1, (char**)&reply
    );
    return reply;
}

I3ipc_reply_version* i3ipc_get_version(void) {
    I3ipc_reply_version* reply = NULL;
    i3ipc_message_and_parse_try(I3IPC_GET_VERSION, I3IPC_TYPE_REPLY_VERSION, NULL, 0, (char**)&reply);
    return reply;
}
void i3ipc_get_version_simple(int* out_major, int* out_minor, int* out_patch) {
    bool prev = i3ipc_set_staticalloc(true);
    I3ipc_reply_version* reply = i3ipc_get_version();
    i3ipc_set_staticalloc(prev);
    if (reply == NULL) return;
    
    if (out_major) *out_major = reply->major;
    if (out_minor) *out_minor = reply->minor;
    if (out_patch) *out_patch = reply->patch;
}

I3ipc_reply_binding_modes* i3ipc_get_binding_modes(void) {
    I3ipc_reply_binding_modes* reply = NULL;
    i3ipc_message_and_parse_try(I3IPC_GET_BINDING_MODES, I3IPC_TYPE_REPLY_BINDING_MODES, NULL, 0, (char**)&reply);
    return reply;
}
I3ipc_reply_config* i3ipc_get_config(void) {
    I3ipc_reply_config* reply = NULL;
    i3ipc_message_and_parse_try(I3IPC_GET_CONFIG, I3IPC_TYPE_REPLY_CONFIG, NULL, 0, (char**)&reply);
    return reply;
}

void i3ipc_send_tick(char const* payload) {
    I3ipc_reply_tick* reply = NULL;
    bool prev = i3ipc_set_staticalloc(true);
    i3ipc_message_and_parse_try(I3IPC_SEND_TICK, I3IPC_TYPE_REPLY_TICK, (char*)payload, -1, (char**)&reply);
    i3ipc_set_staticalloc(prev);
    if (reply == NULL) return;

    if (!reply->success) {
        i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
        return;
    }
}

void i3ipc_sync(int random_value, size_t window) {
    I3ipc_context* context = &i3ipc__global_context;
    if (i3ipc_error_code()) return;

    char const* s = "{\"rnd\":         @,\"window\":                   #}";
    size_t size = strlen(s);
    
    char* buf = NULL;
    i3ipc__context_reserve(context, I3IPC_CONTEXT_PAYLOAD, size + 1, (void**)&buf);
    memcpy(buf, s, size);

    int s0 = -1, s1 = -1;
    for (size_t i = 0; i < size; ++i) {
        if (s[i] == '@') s0 = i;
        if (s[i] == '#') s1 = i;
    }
    assert(s0 && s1);
    
    {unsigned val = random_value;
    for (int i = 0; i < 10; ++i) {
        buf[s0-i] = val % 10;
        val /= 10;
        if (!val) break;
    }}
    {size_t val = window;
    for (int i = 0; i < 20; ++i) {
        buf[s0-i] = val % 10;
        val /= 10;
        if (!val) break;
    }}

    /* Remove whitespace. Not necessary, but normalisation is good. */
    size_t size_new = 0;
    for (size_t i = 0; i < size; ++i) {
        if (buf[i] == ' ') continue;
        buf[size_new++] = buf[i];
    }
    
    I3ipc_reply_sync* reply = NULL;
    bool prev = i3ipc_set_staticalloc(true);
    i3ipc_message_and_parse_try(I3IPC_SYNC, I3IPC_TYPE_REPLY_SYNC, buf, size_new, (char**)&reply);
    i3ipc_set_staticalloc(prev);
    if (reply == NULL) return;

    if (!reply->success) {
        i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
        return;
    }
    return;
}

I3ipc_event* i3ipc_event_next(int timeout_ms) {
    if (i3ipc_error_code()) return NULL;
    
    struct pollfd fd;
    memset(&fd, 0, sizeof(fd));
    fd.fd = i3ipc_event_fd();
    fd.events = POLLIN;

    int code = poll(&fd, 1, timeout_ms);
    if (code == -1) {
        i3ipc__error_errno("while calling poll()");
        i3ipc__error_handle(I3IPC_ERROR_IO);
        return NULL;
    } else if (code == 0) {
        return NULL;
    } else {
        assert(code == 1);
        assert(!(fd.events & POLLNVAL));
        if (fd.events & POLLIN) {
            /* fall through */
        } else if (fd.events & (POLLERR | POLLHUP)) {
            i3ipc__error_handle(I3IPC_ERROR_CLOSED);
            return NULL;
        }
    }

    I3ipc_message* msg;
    {int code = i3ipc_message_receive_reorder_try(I3IPC_EVENT_ANY, &msg);
    if (code) return NULL;}

    int type = i3ipc__message_type_to_event(msg->message_type);
    if (type == -1) {
        fprintf(i3ipc__err, "expected event type, got %s(%x)",
            i3ipc__message_type_str(msg->message_type, true), msg->message_type);
        i3ipc__error_handle(I3IPC_ERROR_MALFORMED);
        return NULL;
    }

    I3ipc_event* reply;
    {int code = i3ipc_parse_try(msg, msg->message_type, type, (char**)&reply);
    if (code) return NULL;}

    reply->type = msg->message_type;
    
    return reply;
}

#endif /* I3IPC_IMPLEMENTATION */
