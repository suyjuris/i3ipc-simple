
#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE 500

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
#define I3IPC_FUZZ
#endif

#define I3IPC_IMPLEMENTATION
#include "../i3ipc.h"

// TODO write command line help pages for this and build.sh

uint64_t i3ipctest__hash_splitmix(uint64_t x) {
    /* see http://xorshift.di.unimi.it/splitmix64.c */
    uint64_t z = x + 0x9e3779b97f4a7c15ul;
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ul;
	z = (z ^ (z >> 27)) * 0x94d049bb133111ebul;
	return z ^ (z >> 31);
}

uint64_t i3ipctest__hash_string(char const* str, int str_size) {
    assert(str);
    if (str == NULL) return 0x5840a1c71c5c7f6aul; /* randomly generated number */
    if (str_size == -1) str_size = strlen(str);
    
    // FNV-1a, 64-bit, see http://www.isthe.com/chongo/tech/comp/fnv/#FNV-1a
    uint64_t hash = 14695981039346656037ul;
    for (int i = 0; i < str_size; ++i) {
        hash = (hash ^ str[i]) * 1099511628211ul;
    }
    return hash;
}

uint64_t i3ipctest__hash_generic_helper(int type_id, int type_flags, char* base, int next_size) {
    assert(type_id != I3IPC_TYPE_EVENT);

    bool is_string_type = (type_id == I3IPC_TYPE_STRING && type_flags == 0)
        || (type_id == I3IPC_TYPE_CHAR && (type_flags & I3IPC_TYPE_ISARRAY));
    
    I3ipc_type type = i3ipc__type_get(type_id);
    if (type_flags & I3IPC_TYPE_ISPTR) {
        char* ptr_base = *(char**)base;
        assert(ptr_base);
        return i3ipctest__hash_generic_helper(type_id, type_flags ^ I3IPC_TYPE_ISPTR, ptr_base, -1);
    } else if (is_string_type) {
        char* str; int str_size;
        if (type_id == I3IPC_TYPE_STRING) {
            str      = *(char**)(base + type.fields[0].offset);
            str_size = *(int*)  (base + type.fields[1].offset);
        } else {
            assert(next_size != -1);
            str = *(char**)base;
            str_size = next_size;
        }
        return i3ipctest__hash_string(str, str_size);
    } else if (type_flags & I3IPC_TYPE_ISARRAY) {
        assert(next_size != -1);
        char* arr_base = *(char**)base;
        assert(arr_base);
        uint64_t hash = 0;
        for (int i = 0; i < next_size; ++i) {
            uint64_t i_hash = i3ipctest__hash_generic_helper(type_id, 0, arr_base + i*type.size, -1);
            hash = i3ipctest__hash_splitmix(hash ^ i_hash);
        }
        return hash;
    } else if (type_flags == 0 && type.is_primitive) {
        uint64_t val = 0;
        switch (type_id) {
        case I3IPC_TYPE_BOOL:  val = *(bool*)base;   break;
        case I3IPC_TYPE_SIZET: val = *(size_t*)base; break;
        case I3IPC_TYPE_INT:   val = *(int*)base;    break;
        case I3IPC_TYPE_FLOAT: val = *(float*)base;  break;
        default: assert(false);
        }
        return i3ipctest__hash_splitmix(val);
    } else if (type_flags == 0) {
        uint64_t hash = 0xe9e1a514652e60d6;
        for (int i = 0; i < type.fields_size; ++i) {
            I3ipc_field field = type.fields[i];
            if (field.flags & I3IPC_TYPE_GROUP_DERIVED) continue;

            uint64_t hash_name = i3ipctest__hash_string(field.full_name, -1);

            bool i_set = true;
            int i_size = -1;
            int i_enum = -1;
            i3ipc__type_readderived(&type, i, base, &i_set, &i_size, &i_enum);

            i_set = i_set && i3ipc__field_is_set(field.type, field.flags, base + field.offset);
            if (!i_set && (field.flags & I3IPC_TYPE_ISOMIT)) continue;
            
            uint64_t hash_value = i_set ?
                i3ipctest__hash_generic_helper(field.type, field.flags & ~I3IPC_TYPE_ISOPT, base + field.offset, i_size)
                : 0x3b62f8aad0455173;

            hash_value ^= i3ipctest__hash_splitmix(hash_value ^ i_enum);
            hash_value ^= i3ipctest__hash_splitmix(hash_value ^ i_size);

            hash ^= i3ipctest__hash_splitmix(hash_name ^ hash_value) ^ hash_value;
        }
        return hash;
    } else {
        assert(false);
    }

}

uint64_t i3ipctest_hash_generic(int type_id, char* obj) {
    return i3ipctest__hash_generic_helper(type_id, 0, obj, -1);
}

void i3ipctest__selfcheck_generic_helper(int type_id, int type_flags, char* base, int next_size) {
    assert(type_id != I3IPC_TYPE_EVENT);

    bool is_string_type = (type_id == I3IPC_TYPE_STRING && type_flags == 0)
        || (type_id == I3IPC_TYPE_CHAR && (type_flags & I3IPC_TYPE_ISARRAY));
    
    I3ipc_type type = i3ipc__type_get(type_id);
    if (type_flags & I3IPC_TYPE_ISPTR) {
        char* ptr_base = *(char**)base;
        if (ptr_base) {
            i3ipctest__selfcheck_generic_helper(type_id, type_flags ^ I3IPC_TYPE_ISPTR, ptr_base, -1);
        }
    } else if (is_string_type) {
        char* str; int str_size;
        if (type_id == I3IPC_TYPE_STRING) {
            str      = *(char**)(base + type.fields[0].offset);
            str_size = *(int*)  (base + type.fields[1].offset);
        } else {
            assert(next_size > -1);
            str = *(char**)base;
            str_size = next_size;
        }
        assert(str || str_size == 0);
        assert(str_size >= 0);
        assert(!str || str[str_size] == 0);
        assert(str || (type_flags & I3IPC_TYPE_ISOPT));
        
    } else if (type_flags & I3IPC_TYPE_ISARRAY) {
        assert(next_size > -1);
        char* arr_base = *(char**)base;
        if (arr_base) {
            for (int i = 0; i < next_size; ++i) {
                i3ipctest__selfcheck_generic_helper(type_id, 0, arr_base + i*type.size, -1);
            }
        } else {
            assert(type_flags & I3IPC_TYPE_ISOPT);
        }        
    } else if ((type_flags & ~I3IPC_TYPE_ISOPT) == 0 && type.is_primitive) {
        /* nothing */
        
    } else if ((type_flags & ~I3IPC_TYPE_ISOPT) == 0) {
        for (int i = 0; i < type.fields_size; ++i) {
            I3ipc_field field = type.fields[i];
            if (field.flags & I3IPC_TYPE_GROUP_DERIVED) continue;

            bool i_set = true;
            int i_size = -1;
            i3ipc__type_readderived(&type, i, base, &i_set, &i_size, NULL);

            if (!i_set) continue;
            i3ipctest__selfcheck_generic_helper(field.type, field.flags, base + field.offset, i_size);
        }
    } else {
        assert(false);
    }
}

void i3ipctest_selfcheck_generic(int type_id, char* obj) {
    i3ipctest__selfcheck_generic_helper(type_id, 0, obj, -1);
}

I3ipc_message* i3ipctest_gen_msg(int type, char* data) {
    char* json_buf;
    size_t json_size;
    FILE* json_f = open_memstream(&json_buf, &json_size);
    assert(json_f);

    I3ipc_message msg;
    memset(&msg, 0, sizeof(msg));
    msg.message_type = 1000 + type;
    
    fwrite(&msg, 1, sizeof(msg), json_f);
    i3ipc_printjson(type, data, json_f);
    fclose(json_f);

    ((I3ipc_message*)json_buf)->message_length = json_size - sizeof(msg);
    
    return (I3ipc_message*)json_buf;
}


I3ipc_message* i3ipctest_read_msg(FILE* inp, size_t max_size) {    
    int type = fgetc(inp) - '0';
    type = type*10 + (fgetc(inp) - '0');
    if (type < I3IPC_TYPE_PRIMITIVE_COUNT || type >= I3IPC_TYPE_COUNT) return NULL;
        
    size_t msg_size = max_size;
    char* msg_buf = (char*)calloc(msg_size, 1);
    I3ipc_message* msg = (I3ipc_message*)msg_buf;
    msg_buf += sizeof(*msg);
    msg_size -= sizeof(*msg);
        
    size_t n = fread(msg_buf, 1, msg_size-1, inp);
    if (ferror(inp)) return NULL;
    msg_buf[n] = 0;

    msg->message_type = 1000 + type;
    msg->message_length = n;
    return msg;
}

enum I3ipctest_codes {
    I3IPCTEST_PARSE_FAIL = 1,
    I3IPCTEST_REPARSE_FAIL,
    I3IPCTEST_REPARSE_NOMATCH_JSON,
    I3IPCTEST_REPARSE_NOMATCH_HASH,
    I3IPCTEST_WRONG_HASH,
    I3IPCTEST_WRONG_FORMAT,
    I3IPCTEST_BAD_MESSAGE,
    I3IPCTEST_FIXPOINT_NOMATCH_JSON
};

int i3ipctest_parse_reparse_msg(I3ipc_message* msg, char** out_data, bool silent, uint64_t* out_hash) {
    int type = msg->message_type - 1000;
    
    char* data;
    i3ipc_set_staticalloc(false);
    if (i3ipc_parse_try(msg, type + 1000, type, &data)) {
        if (!silent) {
            fprintf(i3ipc__err, "while doing original parse\n");
            i3ipc_error_print("Error");
        }
        
        return I3IPCTEST_PARSE_FAIL;
    }

    if (out_data) *out_data = data;

    uint64_t hash2 = i3ipctest_hash_generic(type, data);
    if (out_hash) *out_hash = hash2;
        
    I3ipc_message* msg2 = i3ipctest_gen_msg(type, data);
    size_t msg2_size = sizeof(*msg2) + msg2->message_length;
        
    I3ipc_message* msg2_bak = (I3ipc_message*)calloc(msg2_size, 1);
    assert(msg2_bak);
    memcpy(msg2_bak, msg2, msg2_size);
        
    char* data2;
    i3ipc_set_staticalloc(false);
    if (i3ipc_parse_try(msg2_bak, type + 1000, type, &data2)) {
        if (!silent) {
            fprintf(i3ipc__err, "while doing reparse\n");
            i3ipc_error_print("Error");
        }
        
        free(msg2_bak);
        free(msg2);
        if (!out_data) free(data);
        return I3IPCTEST_REPARSE_FAIL;
    }

    uint64_t hash3 = i3ipctest_hash_generic(type, data2);

    I3ipc_message* msg3 = i3ipctest_gen_msg(type, data2);
    size_t msg3_size = sizeof(*msg3) + msg3->message_length;
        
    if (msg2_size != msg3_size || memcmp(msg2, msg3, msg2_size)) {
        if (!silent) {
            fprintf(stderr, "Error: original json and reparse json do not match\n");
            fputs("<<<<<<<< original json output\n", stderr);
            fwrite(msg2+1, 1, msg2_size - sizeof(*msg2), stderr);
            fputs("\n========\n", stderr);
            fwrite(msg3+1, 1, msg3_size - sizeof(*msg3), stderr);
            fputs("\n>>>>>>>> reparse json output\n", stderr);
        }
        return I3IPCTEST_REPARSE_NOMATCH_JSON;
    }

    if (hash2 != hash3) {
        if (!silent) {
            fprintf(stderr, "Error: reparse hash values do not match (%lx, %lx)\n", (long)hash2, (long)hash3);
        }
        return I3IPCTEST_REPARSE_NOMATCH_HASH;
    }

    free(msg3);
    free(data2);
    free(msg2_bak);
    free(msg2);
    if (!out_data) free(data);

    return 0;
}

int i3ipctest_execute_test_from_file(FILE* inp, bool fuzz_mode) {
    char c = fgetc(inp);

    if (fuzz_mode && c != 'j' && c != 'J') return 0;
    
    if (c == 'j') {
        /* (small) json parsing, check simple consistency */
        i3ipc_set_nopanic(true);
        I3ipc_message* msg = i3ipctest_read_msg(inp, 4096 * 4);
        if (msg == NULL) return 0;
        if (msg->message_type == 1000 + I3IPC_TYPE_EVENT) { free(msg); return 0; }
        int code = i3ipctest_parse_reparse_msg(msg, NULL, fuzz_mode, NULL);
        if (code == I3IPCTEST_PARSE_FAIL || code == I3IPCTEST_REPARSE_FAIL) {
            /* nothing, invalid input is expected */
        } else if (code) {
            if (!fuzz_mode) {
                fprintf(stderr, "Error: exit code %d\n", code);
            }
            abort();
        }
        free(msg);
    } else if (c == 'J') {
        /* execute commands */
        i3ipc_set_nopanic(true);
        i3ipc_set_staticalloc(true);

        int max_size = 4096 * 4;
        char* buf = (char*)calloc(max_size, 1);
        size_t n = fread(buf, 1, max_size-1, inp);
        assert(!ferror(inp));
        buf[n] = 0;

        int fds[4] = {0};
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds))   return 124;
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds+2)) return 125;

        I3ipc_context* context = &i3ipc__global_context;
        context->state = I3IPC_STATE_READY;
        context->sock = fds[0];
        context->sock_events = fds[2];
        context->debug_do_not_write_messages = true;
        context->debug_nodata_is_error = true;

        for (int i = 0; i < (int)(sizeof(fds) / sizeof(fds[0])); ++i) {
            int code = fcntl(fds[i], F_SETFL, O_NONBLOCK);
            assert(!code);
        }
        
        int write_mess  = fds[1];
        int write_event = fds[3];

        for (size_t i = 0; i < n; ++i) {
            char cmd = buf[i];

            if (buf[i] == '\n') continue;
            
            i += i < n;
            i += i < n && buf[i] != '\n';
            size_t beg = i;
            for (; i < n && i < beg + 2048; ++i) {
                if (i-beg < 14 && (cmd == 'm' || cmd == 'e')) continue;
                if (buf[i] == '\n') break;
            }
            size_t end = i+1;
            char* line = &buf[beg];
            size_t line_size = end - beg;
            line[line_size-1] = 0;
            --line_size;

            //mecCsSnVqBty
            if (cmd == 'm' || cmd == 'e') {
                int sock = cmd == 'm' ? write_mess : write_event;
                int code = i3ipc__write_all_try(sock, line, line_size);
                
                if (code == I3IPC_WRITE_ALL_WOULDBLOCK) {
                    /* data would overflow the pipe, abort */
                    break;
                }
                assert(!code);
            } else if (cmd == 'c') {
                i3ipc_run_command(line);
            } else if (cmd == 'C') {
                i3ipc_run_command_simple(line);
            } else if (cmd == 's') {
                int types[16] = {0};
                int types_size = 0;
                for (size_t j = 0; j < line_size && types_size < (int)(sizeof(types)/sizeof(types[0])); ++j) {
                    int type = I3IPC_EVENT_TYPE_BEGIN + (line[j] - '0');
                    if (I3IPC_EVENT_TYPE_BEGIN <= type && type < I3IPC_EVENT_TYPE_END)
                        types[types_size++] = type;
                }
                i3ipc_subscribe(types, types_size);
            } else if (cmd == 'S') {
                if (line_size >= 1) {
                    int type = I3IPC_EVENT_TYPE_BEGIN + (line[0] - '0');
                    if (I3IPC_EVENT_TYPE_BEGIN <= type && type < I3IPC_EVENT_TYPE_END)
                        i3ipc_subscribe_single(type);
                }
            } else if (cmd == 'n') {
                i3ipc_event_next(0);
            } else if (cmd == 'V') {
                i3ipc_get_version_simple(NULL, NULL, NULL);
            } else if (cmd == 'q') {
                if (line_size >= 1) {
                    switch (line[0]) {
                    case 'w': i3ipc_get_workspaces();     break;
                    case 'o': i3ipc_get_outputs();        break;
                    case 't': i3ipc_get_tree();           break;
                    case 'm': i3ipc_get_marks();          break;
                    case 'b': i3ipc_get_bar_config_ids(); break;
                    case 'v': i3ipc_get_version();        break;
                    case 'i': i3ipc_get_binding_modes();  break;
                    case 'c': i3ipc_get_config();         break;
                    default: break; /* nothing */
                    }
                }
            } else if (cmd == 'B') {
                i3ipc_get_bar_config(line);
            } else if (cmd == 't') {
                i3ipc_send_tick(line);
            } else if (cmd == 'y') {
                i3ipc_sync(17, 34);
            }

            int code = i3ipc_error_code();
            if (code == I3IPC_ERROR_FAILED || code == I3IPC_ERROR_MALFORMED) {
                /* nothing, invalid input is expected */
                break;
            } else if (code) {
                if (!fuzz_mode) {
                    i3ipc_error_print(NULL);
                }
                abort();
            }
        }

        free(buf);

    } else if (c == 'h') {
        /* (arbitrarily long) json parsing with hash, additionally check that the hash matches */
        uint64_t hash = 0;
        for (int i = 0; i < 16; ++i) {
            char c = fgetc(inp);
            hash = hash*16 + (
                '0' <= c && c <= '9' ? c - '0' :
                'A' <= c && c <= 'Z' ? c - 'A' + 10 :
                'a' <= c && c <= 'z' ? c - 'a' + 10 : 0);
        }
        if (fgetc(inp) != ',') {
            return I3IPCTEST_WRONG_FORMAT;
        }
        
        long pos_original = ftell(inp);
        fseek(inp, 0, SEEK_END);
        size_t inp_size = ftell(inp) - pos_original;
        fseek(inp, pos_original, SEEK_SET);

        I3ipc_message* msg = i3ipctest_read_msg(inp, inp_size + sizeof(I3ipc_message) - 1);
        if (!msg) return I3IPCTEST_BAD_MESSAGE;
        
        uint64_t msg_hash = 0;
        char* data;
        int code = i3ipctest_parse_reparse_msg(msg, &data, false, &msg_hash);
        if (code) return code;

        i3ipctest_selfcheck_generic(msg->message_type - 1000, data);

        if (msg_hash != hash) {
            fprintf(stderr, "Error: provided hash does not match, should be %016lx, is %016lx\n", (long)hash, (long)msg_hash);
            return I3IPCTEST_WRONG_HASH;
        }
        free(data);
        free(msg);
    } else if (c == 'f') {
        /* fixed-point parsing, check that the generated json matches the parsed json */

        long pos_original = ftell(inp);
        fseek(inp, 0, SEEK_END);
        size_t inp_size = ftell(inp) - pos_original;
        fseek(inp, pos_original, SEEK_SET);
        
        I3ipc_message* msg = i3ipctest_read_msg(inp, inp_size + sizeof(I3ipc_message) - 1);
        if (!msg) return I3IPCTEST_BAD_MESSAGE;
        int type = msg->message_type - 1000;
        
        size_t msg_size = sizeof(*msg) + msg->message_length;
        char* msg_bak = (char*)calloc(1, msg_size);
        memcpy(msg_bak, msg, msg_size);
        
        char* data;
        i3ipc_set_staticalloc(true);
        if (i3ipc_parse_try(msg, type + 1000, type, &data)) {  // @Leak
            fprintf(i3ipc__err, "while doing parse\n");
            i3ipc_error_print("Error");
            return I3IPCTEST_PARSE_FAIL;
        }

        i3ipctest_selfcheck_generic(type, data);
        
        I3ipc_message* msg2 = i3ipctest_gen_msg(type, data);
        size_t msg2_size = sizeof(*msg2) + msg2->message_length;
        if (msg_size != msg2_size || memcmp(msg_bak, msg2, msg_size)) {
            fprintf(stderr, "Error: provided json and parse json do not match\n");
            fputs("<<<<<<<< provided json output\n", stderr);
            fwrite(msg_bak + sizeof(*msg), 1, msg_size  - sizeof(*msg),  stderr);
            fputs("\n========\n", stderr);
            fwrite(msg2+1, 1, msg2_size - sizeof(*msg2), stderr);
            fputs("\n>>>>>>>> parse json output\n", stderr);
            return I3IPCTEST_FIXPOINT_NOMATCH_JSON;
        }

        free(msg2);
        free(msg_bak);
        free(msg);
    }

    return 0;
}

void i3ipctest_execute_test(char const* args[], int args_size, bool fuzz_mode) {
    FILE* inp = stdin;
    if (args_size == 1) {
        if (strcmp(args[0], "-") == 0) {
            /* nothing */
        } else {
            inp = fopen(args[0], "r");
            assert(inp);
        }
        ++args; --args_size;
    }
    assert(args_size == 0);

    exit(i3ipctest_execute_test_from_file(inp, fuzz_mode));
}

void i3ipctest_jsonmin(char* json, size_t* io_json_size) {
    size_t size = *io_json_size;
    if (size < 1024) return;
    
    char stack[1024];
    int stack_size = 0;
    int state = 0;
    size = 1000;
    size_t last = 0;
    for (size_t i = 0; i < size; ++i) {
        char c = json[i];
        if (state == 0 && c == '[') {
            stack[stack_size++] = ']';
            last = i+1;
        } else if (state == 0 && c == '{') {
            stack[stack_size++] = '}';
            last = i+1;
        } else if (state == 0 && (c == ']' || c == '}')) {
            --stack_size;
            last = i+1;
        } else if (state == 0 && c == '"') {
            state = 1;
        } else if (state == 1 && c == '"') {
            state = 0;
        } else if (state == 1 && c == '\\') {
            ++i;
        } else if (state == 0 && c == ',') {
            last = i;
        }
    }
    size = last;
    while (stack_size) json[size++] = stack[--stack_size];
    *io_json_size = size;
    json[size] = 0;
}


void i3ipctest_generate() {
    int types[] = {
        I3IPC_RUN_COMMAND,       I3IPC_TYPE_REPLY_COMMAND,
        I3IPC_GET_WORKSPACES,    I3IPC_TYPE_REPLY_WORKSPACES,
        I3IPC_SUBSCRIBE,         I3IPC_TYPE_REPLY_SUBSCRIBE,
        I3IPC_GET_OUTPUTS,       I3IPC_TYPE_REPLY_OUTPUTS,
        I3IPC_GET_TREE,          I3IPC_TYPE_REPLY_TREE,
        I3IPC_GET_MARKS,         I3IPC_TYPE_REPLY_MARKS,
        I3IPC_GET_BAR_CONFIG,    I3IPC_TYPE_REPLY_BAR_CONFIG_IDS,
        I3IPC_GET_BAR_CONFIG,    I3IPC_TYPE_REPLY_BAR_CONFIG,
        I3IPC_GET_VERSION,       I3IPC_TYPE_REPLY_VERSION,
        I3IPC_GET_BINDING_MODES, I3IPC_TYPE_REPLY_BINDING_MODES,
        I3IPC_GET_CONFIG,        I3IPC_TYPE_REPLY_CONFIG,
        I3IPC_SEND_TICK,         I3IPC_TYPE_REPLY_TICK,
        I3IPC_SYNC,              I3IPC_TYPE_REPLY_SYNC,
        I3IPC_RUN_COMMAND,       -1
    };
    int types_size = sizeof(types) / sizeof(types[0]);

    mkdir("tests", 0755);
    mkdir("tests/base", 0755);
    mkdir("tests/small", 0755);
    
    char const* sample_bar_config = "invalid_bar_config";
    for (int i = 0; i < types_size; i += 2) {
        int type_msg   = types[i];
        int type_reply = types[i+1];
        char const* payload = NULL;
        
        if (type_msg == I3IPC_RUN_COMMAND) {
            if (type_reply != -1) {
                payload = "workspace next; workspace prev; mark --add i3ipctest_mark";
            } else {
                payload = "unmark i3ipctest_mark";
            }
        } else if (type_msg == I3IPC_SUBSCRIBE) {
            payload = "invalid_json"; /* do not actually subscribe */
        } else if (type_reply == I3IPC_TYPE_REPLY_BAR_CONFIG) {
            payload = sample_bar_config;
        } else if (type_msg == I3IPC_SEND_TICK) {
            payload = "i3ipctest_tick";
        } else if (type_msg == I3IPC_SYNC) {
            payload = "invalid_json";
        }

        I3ipc_message* msg;
        int code = i3ipc_message_try(type_msg, payload, -1, &msg);
        if (code) { i3ipc_error_print("Error"); return; }

        if (type_reply == -1) continue;
        
        char buf[64];
        memset(buf, 0, sizeof(buf));

        char* msg_copy = (char*)calloc(msg->message_length, 1);
        memcpy(msg_copy, msg+1, msg->message_length);
        
        {snprintf(buf, sizeof(buf)-1, "tests/small/test%02d", i/2);
        FILE* f = fopen(buf, "wb");
        if (!f) return;

        size_t size = msg->message_length;
        i3ipctest_jsonmin(msg_copy, &size);
        
        fprintf(f, "j%02d", type_reply);
        fwrite(msg_copy, 1, size, f);
        fclose(f);}

        free(msg_copy);
        
        {snprintf(buf, sizeof(buf)-1, "tests/base/test%02d", i/2);
        FILE* f = fopen(buf, "wb");
        if (!f) return;
        
        fprintf(f, "h%016x,%02d", 0, type_reply);
        fwrite((char*)(msg+1), 1, msg->message_length, f);
        
        char* data;
        i3ipc_set_staticalloc(false);
        if (i3ipc_parse_try(msg, type_msg, type_reply, &data)) {
            fprintf(i3ipc__err, "while parsing message for generated test\n");
            i3ipc_error_print("Error");
            exit(5);
        }

        if (type_reply == I3IPC_TYPE_REPLY_BAR_CONFIG_IDS) {
            I3ipc_reply_bar_config_ids* ids = (I3ipc_reply_bar_config_ids*)data;
            if (ids->ids_size > 0) {
                sample_bar_config = ids->ids[0].str;
            }
        }
        
        uint64_t hash = i3ipctest_hash_generic(type_reply, data);
        fseek(f, 0, SEEK_SET);
        fprintf(f, "h%016lx,%02d", (unsigned long)hash, type_reply);
        fclose(f);}
    }

    for (int i = I3IPC_TYPE_PRIMITIVE_COUNT; i < I3IPC_TYPE_COUNT; ++i) {
        I3ipc_type type = i3ipc__type_get(i);
        char const* s;
        if (i == I3IPC_TYPE_STRING) {
            s = "\"\"";
        } else if (type.is_inline && type.fields[0].flags & I3IPC_TYPE_ISARRAY) {
            s =  "[]";
        } else {
            continue;
        }

        char buf[64];
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf)-1, "tests/small/fixpoint%02d", i);
        
        FILE* f = fopen(buf, "wb");
        if (!f) return;

        fprintf(f, "f%02d%s", i, s);
        fclose(f);
    }
    
}

void i3ipctest_primitive() {
    I3ipc_message* msg;
    int code = i3ipc_message_try(I3IPC_GET_TREE, NULL, 0, &msg);
    if (code) { i3ipc_error_print("Error"); return; }

    I3ipc_reply_tree* reply;
    i3ipc_set_staticalloc(true);
    code = i3ipc_parse_try(msg, I3IPC_REPLY_TREE, I3IPC_TYPE_REPLY_TREE, (char**)&reply);
    if (code) { i3ipc_error_print("Error"); return; }

    i3ipc_printjson(I3IPC_TYPE_REPLY_TREE, reply, NULL);
    puts("");
}

#define TERM_RED     "\x1b[31m"
#define TERM_GREEN   "\x1b[32m"
#define TERM_YELLOW  "\x1b[33m"
#define TERM_BLUE    "\x1b[34m"
#define TERM_MAGENTA "\x1b[35m"
#define TERM_CYAN    "\x1b[36m"
#define TERM_WHITE   "\x1b[37m"
#define TERM_RESET   "\x1b[0m"

extern char** environ;

bool global_silent;
int global_counts[3];
int i3ipctest__evaluate_helper(char const* path, struct stat const* stat_unused, int type, struct FTW* ftw_buf) {
    (void)stat_unused;
    if (type != FTW_F) return 0;

    char const* fname = path + ftw_buf->base;
    if (strcmp(fname, "README.txt") == 0) return 0;
    
    pid_t pid = fork();
    if (pid == 0) {
        int fd = open(path, O_RDONLY);
        assert(fd != -1);
        dup2(fd, STDIN_FILENO);
    
        if (global_silent) {
            int fnull = open("/dev/null", O_WRONLY);
            assert(fnull != -1);
            dup2(fnull, STDERR_FILENO);
        } else {
            dup2(STDOUT_FILENO, STDERR_FILENO);
        }
                
        char const* argv[] = {"i3ipc_test", "execute", NULL};
        char* preload = getenv("AFL_PRELOAD");
        if (preload) {
            setenv("LD_PRELOAD", preload, true);
        }
        execve("/proc/self/exe", (char**)argv, environ);
        assert(false);
    } else if (pid != -1) {
        siginfo_t info;
        memset(&info, 0, sizeof(info));
        int code = waitid(P_PID, pid, &info, WEXITED);
        assert(code == 0);
        if (info.si_code == CLD_EXITED && info.si_status) {
            printf("[" TERM_YELLOW "exit" TERM_RESET "]  "); ++global_counts[1];
        } else if (info.si_code == CLD_KILLED || info.si_code == CLD_DUMPED) {
            printf("[" TERM_RED "abrt" TERM_RESET "]  "); ++global_counts[2];
        } else {
            printf("[" TERM_GREEN " ok " TERM_RESET "]  "); ++global_counts[0];
        }
        printf("%s\n", path);
    } else {
        assert(false);
    }

    return 0;
}


void i3ipctest_evaluate(char const* args[], int args_size) {
    bool silent = true;
    if (args_size && strcmp(args[0], "-v") == 0) {
        silent = false;
        ++args; --args_size;
    }
    if (args_size && strcmp(args[0], "--") == 0) {
        ++args; --args_size;
    }

    global_silent = silent;
    memset(global_counts, 0, sizeof(global_counts));
    
    assert(args_size == 1);
    char const* dir_path = args[0];

    int code = nftw(dir_path, &i3ipctest__evaluate_helper, 16, 0);
    assert(code == 0);

    printf("\nTotal: %d ok, %d exit, %d abrt\n", global_counts[0], global_counts[1], global_counts[2]);
}

int main(int argc, char const* argv[]) {
    if (argc <= 1) {
        // @Cleanup update
        fprintf(stderr, "Usage:\n  %s hand|fuzz|generate\n\nStart tests, either the handwritten "
            "ones, or take input from stdin for fuzztesting. To generate initial testcases for "
            "fuzzing, you can use fuzz_start.\n", argv[0]);
        return 1;
    } else if (strcmp(argv[1], "primitive") == 0) {
        i3ipctest_primitive();
    } else if (strcmp(argv[1], "fuzz") == 0) {
        i3ipctest_execute_test(argv+2, argc-2, true);
    } else if (strcmp(argv[1], "execute") == 0) {
        i3ipctest_execute_test(argv+2, argc-2, false);
    } else if (strcmp(argv[1], "generate") == 0) {
        i3ipctest_generate();
    } else if (strcmp(argv[1], "evaluate") == 0) {
        i3ipctest_evaluate(argv+2, argc-2);
    }

    return 0;
}
