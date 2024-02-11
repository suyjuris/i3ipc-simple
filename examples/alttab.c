/* examples/alttab.c */

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

#define I3IPC_IMPLEMENTATION
#include "i3ipc.h"

typedef struct Window {
    size_t id;
    int output;
} Window;

static Window* windows;
static Window* windows_bak;
static int windows_size;
static int windows_bak_size;
static int windows_capacity;
static xcb_key_symbols_t* symbols;

void window_reserve(int capacity) {
    if (windows_capacity < capacity) {
        windows_capacity *= 2;
        if (windows_capacity < capacity) windows_capacity = capacity;
        windows     = realloc(windows,     windows_capacity * sizeof(windows    [0]));
        windows_bak = realloc(windows_bak, windows_capacity * sizeof(windows_bak[0]));
    }
}
        
int window_find(size_t id) {
    int index = -1;
    for (int i = 0; i < windows_size; ++i) {
        if (windows[i].id == id) {
            index = i;
            break;
        }
    }
    return index;
}

bool window_is_normal(I3ipc_node* node) {
    bool xterm = node->window_type_enum == I3IPC_NODE_WINDOW_TYPE_UNKNOWN
        && node->window_properties
        && node->window_properties->instance
        && strcmp(node->window_properties->instance, "xterm") == 0;
    
    return node->window_type_enum == I3IPC_NODE_WINDOW_TYPE_NORMAL || xterm;
}

void window_init_from_tree(I3ipc_node* node) {
    if (window_is_normal(node)) {
        window_reserve(windows_size + 1);
        Window* w = &windows[windows_size++];
        w->id = node->id;
        w->output = -1;
    }

    for (int i = 0; i < node->nodes_size; ++i) {
        window_init_from_tree(&node->nodes[i]);
    }
    for (int i = 0; i < node->floating_nodes_size; ++i) {
        window_init_from_tree(&node->floating_nodes[i]);
    }
}


void window_focus(Window w) {
    int buf_size = 1 + snprintf(NULL, 0, "[con_id=%llu] focus", (long long)w.id);
    char* buf = alloca(buf_size);
    snprintf(buf, buf_size, "[con_id=%llu] focus", (long long)w.id);
    i3ipc_run_command_simple(buf);
}

enum Key_events {
    KEY_EVENT_NONE,
    KEY_EVENT_PRESSED_ALT,
    KEY_EVENT_PRESSED_TAB,
    KEY_EVENT_PRESSED_LEFT,
    KEY_EVENT_PRESSED_RIGHT,
    KEY_EVENT_RELEASED_ALT
};

int main(int argc, char** argv) {
    xcb_connection_t* conn = xcb_connect(NULL, NULL);

    xcb_setup_t const* setup = xcb_get_setup(conn);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;

    xcb_window_t w_id = xcb_generate_id(conn);
    int x_events = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
    xcb_create_window(
        conn, XCB_COPY_FROM_PARENT, w_id, screen->root, 0, 0, 1, 1, 0,
        XCB_WINDOW_CLASS_INPUT_ONLY, screen->root_visual,
        XCB_CW_EVENT_MASK, &x_events
    );

    symbols = xcb_key_symbols_alloc(conn); /* @Leak */

    xcb_mod_mask_t mod_masks[] = {
        XCB_MOD_MASK_1,
        XCB_MOD_MASK_1 | XCB_MOD_MASK_LOCK,
        XCB_MOD_MASK_1 | XCB_MOD_MASK_2,
        XCB_MOD_MASK_1 | XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2
    };
    xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(symbols, XK_Tab);
    if (!keycodes) {
        fprintf(stderr, "Error: Tab key not mapped\n");
        exit(1);
    }
    for (xcb_keycode_t* key = keycodes; key && *key != XCB_NO_SYMBOL; ++key) {
        for (int i = 0; i < sizeof(mod_masks) / sizeof(mod_masks[0]); ++i) {
            xcb_grab_key(conn, false, screen->root, mod_masks[i], *key,
                XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }
    }
    free(keycodes);

    /* @Leak */
    xcb_keycode_t* keycodes_alt = xcb_key_symbols_get_keycode(symbols, XK_Alt_L);
    if (!keycodes_alt) {
        fprintf(stderr, "Error: Alt key not mapped\n");
        exit(1);
    }
    
    xcb_flush(conn);
    
    i3ipc_set_loglevel(0);
    i3ipc_subscribe_single(I3IPC_EVENT_WINDOW);

    int fd_xcb = xcb_get_file_descriptor(conn);
    int fd_i3 = i3ipc_event_fd();

    struct pollfd pfds[2];
    memset(pfds, 0, sizeof(pfds));
    pfds[0].fd = fd_xcb;
    pfds[0].events = POLLIN;
    pfds[1].fd = fd_i3;
    pfds[1].events = POLLIN;

    /* window initialisation */
    I3ipc_reply_tree* tree = i3ipc_get_tree();
    window_init_from_tree(&tree->root);
    free(tree);

    int state = 0;
    int tabbing_index = 0;
    
    while (true) {
        int code = poll(pfds, sizeof(pfds) / sizeof(pfds[0]), -1);
        if (code == -1) {
            perror("Error");
            fprintf(stderr, "Error: while doing poll()\n");
            exit(1);
        }

        {I3ipc_event* ev_any = i3ipc_event_next(0);
        if (ev_any) {
            if (ev_any->type == I3IPC_EVENT_WINDOW) {
                I3ipc_event_window* ev = &ev_any->window;
                
                if (window_is_normal(&ev->container)) {
                    if (ev->change_enum == I3IPC_WINDOW_CHANGE_NEW) {
                        int index = window_find(ev->container.id);
                        if (index != -1) continue;

                        window_reserve(windows_size + 1);
                        Window* w = &windows[windows_size++];
                        w->id = ev->container.id;
                        w->output = -1;
                    } else if (ev->change_enum == I3IPC_WINDOW_CHANGE_CLOSE) {
                        int index = window_find(ev->container.id);
                        if (index == -1) continue;
                        for (int i = index; i+1 < windows_size; ++i) {
                            windows[i] = windows[i+1];
                        }
                        --windows_size;
                    } else if (ev->change_enum == I3IPC_WINDOW_CHANGE_FOCUS) {
                        if (state == 0) {
                            int index = window_find(ev->container.id);
                            if (index == -1) continue;
                            Window tmp = windows[index];
                            for (int i = index; i+1 < windows_size; ++i) {
                                windows[i] = windows[i+1];
                            }
                            windows[windows_size-1] = tmp;
                        }
                    } else {
                        /* nothing */
                    }
                }
            } else {
                assert(false);
            }

            free(ev_any);
        }}

        
        int key_event = KEY_EVENT_NONE;
        
        xcb_generic_event_t* ev_any = xcb_poll_for_event(conn);
        if (ev_any) {
            uint8_t type = ev_any->response_type & ~0x80;
            if (type == XCB_KEY_PRESS) {
                xcb_key_press_event_t* ev = (xcb_key_press_event_t*)ev_any;
                xcb_keysym_t keysym = xcb_key_press_lookup_keysym(symbols, ev, 0);
                if (keysym == XK_Alt_L) {
                    key_event = KEY_EVENT_PRESSED_ALT;
                } else if (keysym == XK_Tab) {
                    key_event = KEY_EVENT_PRESSED_TAB;
                } else if (keysym == XK_Left) {
                    key_event = KEY_EVENT_PRESSED_LEFT;
                } else if (keysym == XK_Right) {
                    key_event = KEY_EVENT_PRESSED_RIGHT;
                }
            } else if (type == XCB_KEY_RELEASE) {
                xcb_key_release_event_t* ev = (xcb_key_release_event_t*)ev_any;
                xcb_keysym_t keysym = xcb_key_release_lookup_keysym(symbols, ev, 0);
                if (keysym == XK_Alt_L) {
                    key_event = KEY_EVENT_RELEASED_ALT;
                }
            }
            
            free(ev_any);
        }

        int do_tab_diff = 0;
        bool do_commit = false;
        if (state == 0) {
            if (key_event == KEY_EVENT_PRESSED_TAB) {
                xcb_grab_keyboard_cookie_t cookie = xcb_grab_keyboard_unchecked(conn, 0, screen->root, XCB_CURRENT_TIME,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
                xcb_grab_keyboard_reply_t* reply = xcb_grab_keyboard_reply(conn, cookie, NULL);
                if (reply->status != XCB_GRAB_STATUS_SUCCESS) {
                    fprintf(stderr, "Error: could not grab keyboard\n");
                    exit(2);
                }

                memcpy(windows_bak, windows, windows_size * sizeof(windows[0]));
                windows_bak_size = windows_size;
                tabbing_index = windows_size - 1;
                do_tab_diff = -1;
                state = 1;
            }
        } else if (state == 1) {
            if (key_event == KEY_EVENT_RELEASED_ALT) {
                xcb_ungrab_keyboard(conn, XCB_CURRENT_TIME);
                xcb_flush(conn);
                state = 0;
                do_commit = true;
            } else if (key_event == KEY_EVENT_PRESSED_TAB || key_event == KEY_EVENT_PRESSED_RIGHT) {
                do_tab_diff = -1;
            } else if (key_event == KEY_EVENT_PRESSED_LEFT) {
                do_tab_diff = 1;
            }
        }

        if (do_tab_diff && windows_bak_size) {
            tabbing_index = (tabbing_index + do_tab_diff + windows_bak_size) % windows_bak_size;
            window_focus(windows_bak[tabbing_index]);
        }
        if (do_commit && 0 <= tabbing_index && tabbing_index < windows_bak_size) {
            size_t id = windows_bak[tabbing_index].id;
            int index = -1;
            for (int i = 0; i < windows_size; ++i) {
                if (windows[i].id == id) {
                    index = i;
                    break;
                }
            }
            if (index >= 0) {
                Window tmp = windows[index];
                for (int i = index; i+1 < windows_size; ++i) {
                    windows[i] = windows[i+1];
                }
                windows[windows_size-1] = tmp;
            }
        }
    }
}
