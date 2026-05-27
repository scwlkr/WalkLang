#include "walk_runtime.h"
#include "platform/walk_platform.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32)
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#endif

#if defined(_WIN32) && !defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static int walk_rt_host_argc = 0;
static char **walk_rt_host_argv = NULL;

void walk_rt_init(int argc, char **argv) {
    walk_rt_host_argc = argc;
    walk_rt_host_argv = argv;
}

void *walk_rt_alloc_array(WalkSize len, size_t item_size) {
    if (len <= 0) { return NULL; }
    void *items = calloc((size_t)len, item_size);
    if (items == NULL) {
        fprintf(stderr, "walk runtime error: out of memory\n");
        exit(1);
    }
    return items;
}

void walk_rt_panic(const char *message) {
    fprintf(stderr, "walk runtime error: %s\n", message);
    exit(1);
}

static unsigned long long walk_rt_random_state = 0;

static WALK_UNUSED unsigned long long walk_rt_random_seed(void) {
    unsigned long long seed = (unsigned long long)time(NULL);
    seed ^= ((unsigned long long)clock() << 32);
    seed ^= (unsigned long long)(uintptr_t)&seed;
#if defined(_WIN32)
    seed ^= ((unsigned long long)_getpid() << 16);
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        seed ^= ((unsigned long long)tv.tv_sec << 32) ^ (unsigned long long)tv.tv_usec;
    }
    seed ^= ((unsigned long long)getpid() << 16);
#endif
    if (seed == 0) { seed = 0x9e3779b97f4a7c15ULL; }
    return seed;
}

static WALK_UNUSED unsigned long long walk_rt_random_next(void) {
    if (walk_rt_random_state == 0) { walk_rt_random_state = walk_rt_random_seed(); }
    unsigned long long x = walk_rt_random_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    walk_rt_random_state = x;
    return x * 2685821657736338717ULL;
}

WalkInt walk_rt_random_int(WalkInt min, WalkInt max) {
    if (max < min) { return min; }
    return min + (WalkInt)(walk_rt_random_next() % (unsigned long long)(max - min + 1));
}

WalkFloat walk_rt_random_float(WalkFloat min, WalkFloat max) {
    if (max < min) { return min; }
    const double unit = (double)(walk_rt_random_next() >> 11) * (1.0 / 9007199254740992.0);
    return min + (max - min) * unit;
}

static WALK_UNUSED WalkSize walk_rt_random_index(WalkSize len) {
    if (len <= 0) { walk_rt_panic("random.choice on empty array"); }
    return (WalkSize)(walk_rt_random_next() % (unsigned long long)len);
}

static WALK_UNUSED WalkString walk_rt_copy_string(const char *value);

WalkInt walk_rt_string_len(WalkString value) {
    return value == NULL ? 0 : (WalkInt)strlen(value);
}

WalkString walk_rt_string_at(WalkString value, WalkInt index) {
    WalkSize len = (WalkSize)walk_rt_string_len(value);
    if (index < 0 || index >= len) { walk_rt_panic("string index out of range"); }
    char *out = (char *)malloc(2);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    out[0] = value[index];
    out[1] = '\0';
    return out;
}

WalkBool walk_rt_string_contains(WalkString text, WalkString item) {
    if (text == NULL) { text = ""; }
    if (item == NULL) { item = ""; }
    return strstr(text, item) != NULL;
}

WalkString walk_rt_string_concat(WalkString left, WalkString right) {
    if (left == NULL) { left = ""; }
    if (right == NULL) { right = ""; }
    size_t left_len = strlen(left);
    size_t right_len = strlen(right);
    if (left_len > ((size_t)-1) - right_len - 1) { walk_rt_panic("out of memory"); }
    char *out = (char *)malloc(left_len + right_len + 1);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    memcpy(out, left, left_len);
    memcpy(out + left_len, right, right_len);
    out[left_len + right_len] = '\0';
    return out;
}

WalkString walk_rt_string_lower(WalkString text) {
    if (text == NULL) { text = ""; }
    size_t len = strlen(text);
    char *out = (char *)malloc(len + 1);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    for (size_t i = 0; i < len; i++) {
        char ch = text[i];
        out[i] = (ch >= 'A' && ch <= 'Z') ? (char)(ch - 'A' + 'a') : ch;
    }
    out[len] = '\0';
    return out;
}

WalkArrayString walk_rt_string_split(WalkString text, WalkString sep) {
    if (text == NULL) { text = ""; }
    if (sep == NULL) { sep = ""; }
    size_t sep_len = strlen(sep);
    if (sep_len == 0) {
        WalkString *items = (WalkString *)walk_rt_alloc_array(1, sizeof(WalkString));
        items[0] = walk_rt_copy_string(text);
        return (WalkArrayString){items, 1};
    }
    WalkSize count = 1;
    const char *scan = text;
    while ((scan = strstr(scan, sep)) != NULL) {
        count++;
        scan += sep_len;
    }
    WalkString *items = (WalkString *)walk_rt_alloc_array(count, sizeof(WalkString));
    const char *start = text;
    WalkSize index = 0;
    while (true) {
        const char *hit = strstr(start, sep);
        size_t part_len = hit == NULL ? strlen(start) : (size_t)(hit - start);
        char *part = (char *)malloc(part_len + 1);
        if (part == NULL) { walk_rt_panic("out of memory"); }
        memcpy(part, start, part_len);
        part[part_len] = '\0';
        items[index++] = part;
        if (hit == NULL) { break; }
        start = hit + sep_len;
    }
    return (WalkArrayString){items, count};
}

WalkString walk_rt_string_replace(WalkString text, WalkString from, WalkString to) {
    if (text == NULL) { text = ""; }
    if (from == NULL) { from = ""; }
    if (to == NULL) { to = ""; }
    size_t from_len = strlen(from);
    if (from_len == 0) { return walk_rt_copy_string(text); }
    size_t text_len = strlen(text);
    size_t to_len = strlen(to);
    WalkSize count = 0;
    const char *scan = text;
    while ((scan = strstr(scan, from)) != NULL) {
        count++;
        scan += from_len;
    }
    size_t out_len = text_len;
    if (to_len >= from_len) {
        size_t growth = to_len - from_len;
        if (count > 0 && growth > (((size_t)-1) - out_len) / (size_t)count) { walk_rt_panic("out of memory"); }
        out_len += growth * (size_t)count;
    } else {
        out_len -= (from_len - to_len) * (size_t)count;
    }
    char *out = (char *)malloc(out_len + 1);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    const char *input = text;
    char *write = out;
    while (true) {
        const char *hit = strstr(input, from);
        if (hit == NULL) {
            size_t tail_len = strlen(input);
            memcpy(write, input, tail_len);
            write += tail_len;
            break;
        }
        size_t prefix_len = (size_t)(hit - input);
        memcpy(write, input, prefix_len);
        write += prefix_len;
        memcpy(write, to, to_len);
        write += to_len;
        input = hit + from_len;
    }
    *write = '\0';
    return out;
}

WalkString walk_rt_format_int(WalkInt value) {
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
    if (len < 0 || (size_t)len >= sizeof(buffer)) { walk_rt_panic("format failed"); }
    char *out = (char *)malloc((size_t)len + 1);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    memcpy(out, buffer, (size_t)len + 1);
    return out;
}

WalkString walk_rt_format_float(WalkFloat value) {
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer), "%g", (double)value);
    if (len < 0 || (size_t)len >= sizeof(buffer)) { walk_rt_panic("format failed"); }
    char *out = (char *)malloc((size_t)len + 1);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    memcpy(out, buffer, (size_t)len + 1);
    return out;
}

WalkString walk_rt_format_bool(WalkBool value) {
    return value ? "true" : "false";
}

WalkString walk_rt_format_string(WalkString value) {
    return value == NULL ? "null" : value;
}

static WALK_UNUSED WalkString walk_rt_copy_string(const char *value) {
    if (value == NULL) { value = ""; }
    size_t len = strlen(value);
    char *out = (char *)malloc(len + 1);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    memcpy(out, value, len + 1);
    return out;
}

WalkArrayInt walk_rt_array_push_int(WalkArrayInt array, WalkInt item) {
    WalkInt *items = (WalkInt *)walk_rt_alloc_array(array.len + 1, sizeof(WalkInt));
    for (WalkSize i = 0; i < array.len; i++) { items[i] = array.items[i]; }
    items[array.len] = item;
    return (WalkArrayInt){items, array.len + 1};
}

WalkArrayFloat walk_rt_array_push_float(WalkArrayFloat array, WalkFloat item) {
    WalkFloat *items = (WalkFloat *)walk_rt_alloc_array(array.len + 1, sizeof(WalkFloat));
    for (WalkSize i = 0; i < array.len; i++) { items[i] = array.items[i]; }
    items[array.len] = item;
    return (WalkArrayFloat){items, array.len + 1};
}

WalkArrayBool walk_rt_array_push_bool(WalkArrayBool array, WalkBool item) {
    WalkBool *items = (WalkBool *)walk_rt_alloc_array(array.len + 1, sizeof(WalkBool));
    for (WalkSize i = 0; i < array.len; i++) { items[i] = array.items[i]; }
    items[array.len] = item;
    return (WalkArrayBool){items, array.len + 1};
}

WalkArrayString walk_rt_array_push_string(WalkArrayString array, WalkString item) {
    WalkString *items = (WalkString *)walk_rt_alloc_array(array.len + 1, sizeof(WalkString));
    for (WalkSize i = 0; i < array.len; i++) { items[i] = array.items[i]; }
    items[array.len] = item;
    return (WalkArrayString){items, array.len + 1};
}

WalkBool walk_rt_array_contains_int(WalkArrayInt array, WalkInt item) {
    for (WalkSize i = 0; i < array.len; i++) { if (array.items[i] == item) { return true; } }
    return false;
}

WalkBool walk_rt_array_contains_float(WalkArrayFloat array, WalkFloat item) {
    for (WalkSize i = 0; i < array.len; i++) { if (array.items[i] == item) { return true; } }
    return false;
}

WalkBool walk_rt_array_contains_bool(WalkArrayBool array, WalkBool item) {
    for (WalkSize i = 0; i < array.len; i++) { if (array.items[i] == item) { return true; } }
    return false;
}

WalkBool walk_rt_array_contains_string(WalkArrayString array, WalkString item) {
    for (WalkSize i = 0; i < array.len; i++) { if (strcmp(array.items[i], item) == 0) { return true; } }
    return false;
}

WalkMapStringArrayString walk_rt_map_string_array_string_empty(void) {
    return (WalkMapStringArrayString){NULL, 0};
}

static WalkSize walk_rt_map_string_array_string_find(WalkMapStringArrayString table, WalkString key) {
    if (key == NULL) { key = ""; }
    for (WalkSize i = 0; i < table.len; i++) {
        WalkString entry_key = table.entries[i].key == NULL ? "" : table.entries[i].key;
        if (strcmp(entry_key, key) == 0) { return i; }
    }
    return -1;
}

WalkBool walk_rt_map_string_array_string_has(WalkMapStringArrayString table, WalkString key) {
    return walk_rt_map_string_array_string_find(table, key) >= 0;
}

WalkArrayString walk_rt_map_string_array_string_get(WalkMapStringArrayString table, WalkString key) {
    WalkSize index = walk_rt_map_string_array_string_find(table, key);
    if (index < 0) { return (WalkArrayString){NULL, 0}; }
    return table.entries[index].value;
}

WalkMapStringArrayString walk_rt_map_string_array_string_set(WalkMapStringArrayString table, WalkString key, WalkArrayString value) {
    if (key == NULL) { key = ""; }
    WalkSize found = walk_rt_map_string_array_string_find(table, key);
    WalkSize len = found >= 0 ? table.len : table.len + 1;
    WalkMapStringArrayStringEntry *entries = (WalkMapStringArrayStringEntry *)walk_rt_alloc_array(len, sizeof(WalkMapStringArrayStringEntry));
    for (WalkSize i = 0; i < table.len; i++) {
        entries[i] = table.entries[i];
    }
    if (found >= 0) {
        entries[found].value = value;
    } else {
        entries[table.len] = (WalkMapStringArrayStringEntry){key, value};
    }
    return (WalkMapStringArrayString){entries, len};
}

WalkArrayString walk_rt_map_string_array_string_keys(WalkMapStringArrayString table) {
    WalkString *items = (WalkString *)walk_rt_alloc_array(table.len, sizeof(WalkString));
    for (WalkSize i = 0; i < table.len; i++) {
        items[i] = table.entries[i].key;
    }
    return (WalkArrayString){items, table.len};
}

WalkMapStringArrayString walk_rt_map_string_array_string_push(WalkMapStringArrayString table, WalkString key, WalkString value) {
    WalkArrayString values = walk_rt_map_string_array_string_get(table, key);
    WalkArrayString next = walk_rt_array_push_string(values, value);
    return walk_rt_map_string_array_string_set(table, key, next);
}

WalkInt walk_rt_array_choice_int(WalkArrayInt array) { return array.items[walk_rt_random_index(array.len)]; }
WalkFloat walk_rt_array_choice_float(WalkArrayFloat array) { return array.items[walk_rt_random_index(array.len)]; }
WalkBool walk_rt_array_choice_bool(WalkArrayBool array) { return array.items[walk_rt_random_index(array.len)]; }
WalkString walk_rt_array_choice_string(WalkArrayString array) { return array.items[walk_rt_random_index(array.len)]; }

WalkString walk_rt_input_line(WalkString prompt) {
    if (prompt != NULL) {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    size_t cap = 64;
    size_t len = 0;
    char *buffer = (char *)malloc(cap);
    if (buffer == NULL) {
        fprintf(stderr, "walk runtime error: out of memory\n");
        exit(1);
    }
    for (;;) {
        int ch = fgetc(stdin);
        if (ch == EOF) {
            if (ferror(stdin)) {
                free(buffer);
                fprintf(stderr, "walk runtime error: stdin read failed\n");
                exit(1);
            }
            if (len == 0) {
                free(buffer);
                fprintf(stderr, "walk runtime error: input reached EOF\n");
                exit(1);
            }
            break;
        }
        if (ch == '\n') { break; }
        if (ch == '\r') {
            int next = fgetc(stdin);
            if (next == '\n') { break; }
            if (next == EOF && ferror(stdin)) {
                free(buffer);
                fprintf(stderr, "walk runtime error: stdin read failed\n");
                exit(1);
            }
            if (next != EOF) { ungetc(next, stdin); }
        }
        if (len + 1 >= cap) {
            if (cap > ((size_t)-1) / 2) {
                free(buffer);
                fprintf(stderr, "walk runtime error: out of memory\n");
                exit(1);
            }
            size_t next_cap = cap * 2;
            char *next_buffer = (char *)realloc(buffer, next_cap);
            if (next_buffer == NULL) {
                free(buffer);
                fprintf(stderr, "walk runtime error: out of memory\n");
                exit(1);
            }
            buffer = next_buffer;
            cap = next_cap;
        }
        buffer[len++] = (char)ch;
    }
    buffer[len] = '\0';
    return buffer;
}

void walk_rt_print_int(WalkInt value) { printf("%lld\n", (long long)value); }
void walk_rt_print_float(WalkFloat value) { printf("%g\n", (double)value); }
void walk_rt_print_bool(WalkBool value) { printf("%s\n", value ? "true" : "false"); }
void walk_rt_print_string(WalkString value) { printf("%s\n", value == NULL ? "null" : value); }

void walk_rt_io_write(WalkString value) {
    fputs(value == NULL ? "null" : value, stdout);
    fflush(stdout);
}

void walk_rt_io_write_line(WalkString value) {
    fprintf(stdout, "%s\n", value == NULL ? "null" : value);
    fflush(stdout);
}

void walk_rt_io_error_line(WalkString value) {
    fprintf(stderr, "%s\n", value == NULL ? "null" : value);
    fflush(stderr);
}

static WALK_UNUSED IOReadResult walk_rt_io_read_ok(WalkString value) {
    return (IOReadResult){.ok = true, .value = value, .error = ""};
}

static WALK_UNUSED IOReadResult walk_rt_io_read_error(WalkString error) {
    return (IOReadResult){.ok = false, .value = "", .error = error};
}

IOReadResult walk_rt_io_read_line(void) {
    size_t cap = 64;
    size_t len = 0;
    char *buffer = (char *)malloc(cap);
    if (buffer == NULL) { walk_rt_panic("out of memory"); }
    for (;;) {
        int ch = fgetc(stdin);
        if (ch == EOF) {
            if (ferror(stdin)) {
                free(buffer);
                return walk_rt_io_read_error("stdin read failed");
            }
            if (len == 0) {
                free(buffer);
                return walk_rt_io_read_error("eof");
            }
            break;
        }
        if (ch == '\n') { break; }
        if (ch == '\r') {
            int next = fgetc(stdin);
            if (next == '\n') { break; }
            if (next == EOF && ferror(stdin)) {
                free(buffer);
                return walk_rt_io_read_error("stdin read failed");
            }
            if (next != EOF) { ungetc(next, stdin); }
        }
        if (len + 1 >= cap) {
            if (cap > ((size_t)-1) / 2) {
                free(buffer);
                walk_rt_panic("out of memory");
            }
            size_t next_cap = cap * 2;
            char *next_buffer = (char *)realloc(buffer, next_cap);
            if (next_buffer == NULL) {
                free(buffer);
                walk_rt_panic("out of memory");
            }
            buffer = next_buffer;
            cap = next_cap;
        }
        buffer[len++] = (char)ch;
    }
    buffer[len] = '\0';
    return walk_rt_io_read_ok(buffer);
}

IOReadResult walk_rt_io_read_all(void) {
    size_t cap = 64;
    size_t len = 0;
    char *buffer = (char *)malloc(cap);
    if (buffer == NULL) { walk_rt_panic("out of memory"); }
    for (;;) {
        int ch = fgetc(stdin);
        if (ch == EOF) {
            if (ferror(stdin)) {
                free(buffer);
                return walk_rt_io_read_error("stdin read failed");
            }
            break;
        }
        if (len + 1 >= cap) {
            if (cap > ((size_t)-1) / 2) {
                free(buffer);
                walk_rt_panic("out of memory");
            }
            size_t next_cap = cap * 2;
            char *next_buffer = (char *)realloc(buffer, next_cap);
            if (next_buffer == NULL) {
                free(buffer);
                walk_rt_panic("out of memory");
            }
            buffer = next_buffer;
            cap = next_cap;
        }
        buffer[len++] = (char)ch;
    }
    buffer[len] = '\0';
    return walk_rt_io_read_ok(buffer);
}

WalkBool walk_rt_term_is_tty(void) {
    return walk_platform_stdout_is_tty();
}

static WALK_UNUSED WalkBool walk_rt_term_stdin_is_tty(void) {
    return walk_platform_stdin_is_tty();
}

static WALK_UNUSED WalkBool walk_rt_term_force_ansi(void) {
    const char *value = getenv("CLICOLOR_FORCE");
    return value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
}

static WALK_UNUSED WalkBool walk_rt_term_ansi_enabled(void) {
    if (getenv("NO_COLOR") != NULL) { return false; }
    if (walk_rt_term_force_ansi()) { return true; }
    const char *term = getenv("TERM");
    if (term != NULL && strcmp(term, "dumb") == 0) { return false; }
    if (!walk_rt_term_is_tty()) { return false; }
    return walk_platform_enable_ansi();
}

static WALK_UNUSED const char *walk_rt_term_color_code(WalkString name, WalkBool background) {
    if (name == NULL) { return NULL; }
    if (strcmp(name, "default") == 0) { return background ? "49" : "39"; }
    if (strcmp(name, "black") == 0) { return background ? "40" : "30"; }
    if (strcmp(name, "red") == 0) { return background ? "41" : "31"; }
    if (strcmp(name, "green") == 0) { return background ? "42" : "32"; }
    if (strcmp(name, "yellow") == 0) { return background ? "43" : "33"; }
    if (strcmp(name, "blue") == 0) { return background ? "44" : "34"; }
    if (strcmp(name, "magenta") == 0) { return background ? "45" : "35"; }
    if (strcmp(name, "cyan") == 0) { return background ? "46" : "36"; }
    if (strcmp(name, "white") == 0) { return background ? "47" : "37"; }
    return NULL;
}

static WALK_UNUSED const char *walk_rt_term_style_code(WalkString name) {
    if (name == NULL) { return NULL; }
    if (strcmp(name, "bold") == 0) { return "1"; }
    if (strcmp(name, "dim") == 0) { return "2"; }
    if (strcmp(name, "italic") == 0) { return "3"; }
    if (strcmp(name, "underline") == 0) { return "4"; }
    if (strcmp(name, "reverse") == 0) { return "7"; }
    if (strcmp(name, "normal") == 0 || strcmp(name, "reset") == 0) { return "0"; }
    return NULL;
}

static WALK_UNUSED void walk_rt_term_emit_code(const char *code) {
    if (!walk_rt_term_ansi_enabled()) { return; }
    fprintf(stdout, "\033[%sm", code);
    fflush(stdout);
}

void walk_rt_term_color(WalkString name) {
    const char *code = walk_rt_term_color_code(name, false);
    if (code == NULL) { walk_rt_panic("term color unknown"); }
    walk_rt_term_emit_code(code);
}

void walk_rt_term_background(WalkString name) {
    const char *code = walk_rt_term_color_code(name, true);
    if (code == NULL) { walk_rt_panic("term background unknown"); }
    walk_rt_term_emit_code(code);
}

void walk_rt_term_style(WalkString name) {
    const char *code = walk_rt_term_style_code(name);
    if (code == NULL) { walk_rt_panic("term style unknown"); }
    walk_rt_term_emit_code(code);
}

void walk_rt_term_reset(void) {
    walk_rt_term_emit_code("0");
}

void walk_rt_term_clear(void) {
    if (!walk_rt_term_ansi_enabled()) { return; }
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
}

void walk_rt_term_move(WalkInt column, WalkInt row) {
    if (column < 1 || row < 1) { walk_rt_panic("term move invalid"); }
    if (!walk_rt_term_ansi_enabled()) { return; }
    fprintf(stdout, "\033[%lld;%lldH", (long long)row, (long long)column);
    fflush(stdout);
}

static WALK_UNUSED WalkInt walk_rt_term_env_dimension(const char *name, WalkInt fallback) {
    const char *value = getenv(name);
    if (value == NULL || value[0] == '\0') { return fallback; }
    errno = 0;
    char *end = NULL;
    long long parsed = strtoll(value, &end, 10);
    if (end != value && *end == '\0' && errno != ERANGE && parsed > 0) { return (WalkInt)parsed; }
    return fallback;
}

WalkInt walk_rt_term_width(void) {
    WalkInt width = walk_platform_terminal_width();
    if (width > 0) { return width; }
    return walk_rt_term_env_dimension("COLUMNS", 80);
}

WalkInt walk_rt_term_height(void) {
    WalkInt height = walk_platform_terminal_height();
    if (height > 0) { return height; }
    return walk_rt_term_env_dimension("LINES", 24);
}

IOReadResult walk_rt_term_read_key(void) {
    if (!walk_rt_term_stdin_is_tty()) { return walk_rt_io_read_error("terminal not interactive"); }
    char ch = '\0';
    const char *error = "";
    if (!walk_platform_read_key(&ch, &error)) { return walk_rt_io_read_error(error); }
    char *buffer = (char *)malloc(2);
    if (buffer == NULL) { walk_rt_panic("out of memory"); }
    buffer[0] = ch;
    buffer[1] = '\0';
    return walk_rt_io_read_ok(buffer);
}

static WALK_UNUSED WalkBool walk_rt_utf8_cont(unsigned char ch) {
    return (ch & 0xC0) == 0x80;
}

static WALK_UNUSED WalkBool walk_rt_is_valid_utf8(WalkString text) {
    if (text == NULL) { return false; }
    const unsigned char *bytes = (const unsigned char *)text;
    size_t len = strlen(text);
    size_t i = 0;
    while (i < len) {
        unsigned char ch = bytes[i];
        if (ch <= 0x7F) { i++; continue; }
        if (ch >= 0xC2 && ch <= 0xDF) {
            if (i + 1 >= len || !walk_rt_utf8_cont(bytes[i + 1])) { return false; }
            i += 2;
            continue;
        }
        if (ch == 0xE0) {
            if (i + 2 >= len || bytes[i + 1] < 0xA0 || bytes[i + 1] > 0xBF || !walk_rt_utf8_cont(bytes[i + 2])) { return false; }
            i += 3;
            continue;
        }
        if ((ch >= 0xE1 && ch <= 0xEC) || (ch >= 0xEE && ch <= 0xEF)) {
            if (i + 2 >= len || !walk_rt_utf8_cont(bytes[i + 1]) || !walk_rt_utf8_cont(bytes[i + 2])) { return false; }
            i += 3;
            continue;
        }
        if (ch == 0xED) {
            if (i + 2 >= len || bytes[i + 1] < 0x80 || bytes[i + 1] > 0x9F || !walk_rt_utf8_cont(bytes[i + 2])) { return false; }
            i += 3;
            continue;
        }
        if (ch == 0xF0) {
            if (i + 3 >= len || bytes[i + 1] < 0x90 || bytes[i + 1] > 0xBF || !walk_rt_utf8_cont(bytes[i + 2]) || !walk_rt_utf8_cont(bytes[i + 3])) { return false; }
            i += 4;
            continue;
        }
        if (ch >= 0xF1 && ch <= 0xF3) {
            if (i + 3 >= len || !walk_rt_utf8_cont(bytes[i + 1]) || !walk_rt_utf8_cont(bytes[i + 2]) || !walk_rt_utf8_cont(bytes[i + 3])) { return false; }
            i += 4;
            continue;
        }
        if (ch == 0xF4) {
            if (i + 3 >= len || bytes[i + 1] < 0x80 || bytes[i + 1] > 0x8F || !walk_rt_utf8_cont(bytes[i + 2]) || !walk_rt_utf8_cont(bytes[i + 3])) { return false; }
            i += 4;
            continue;
        }
        return false;
    }
    return true;
}

static WALK_UNUSED WalkString walk_rt_file_read_try_value(WalkString path, WalkString *error) {
    *error = "";
    if (path == NULL || path[0] == '\0') { *error = "file path empty"; return ""; }
    FILE *file = fopen(path, "rb");
    if (file == NULL) { *error = "file read failed"; return ""; }
    size_t cap = 64;
    size_t len = 0;
    char *buffer = (char *)malloc(cap);
    if (buffer == NULL) {
        fclose(file);
        walk_rt_panic("out of memory");
    }
    for (;;) {
        int ch = fgetc(file);
        if (ch == EOF) {
            if (ferror(file)) {
                fclose(file);
                free(buffer);
                *error = "file read failed";
                return "";
            }
            break;
        }
        if (ch == '\0') {
            fclose(file);
            free(buffer);
            *error = "file contains null byte";
            return "";
        }
        if (len + 1 >= cap) {
            if (cap > ((size_t)-1) / 2) {
                fclose(file);
                free(buffer);
                walk_rt_panic("out of memory");
            }
            size_t next_cap = cap * 2;
            char *next_buffer = (char *)realloc(buffer, next_cap);
            if (next_buffer == NULL) {
                fclose(file);
                free(buffer);
                walk_rt_panic("out of memory");
            }
            buffer = next_buffer;
            cap = next_cap;
        }
        buffer[len++] = (char)ch;
    }
    if (fclose(file) != 0) {
        free(buffer);
        *error = "file read failed";
        return "";
    }
    buffer[len] = '\0';
    if (!walk_rt_is_valid_utf8(buffer)) {
        free(buffer);
        *error = "file invalid utf-8";
        return "";
    }
    return buffer;
}

WalkString walk_rt_file_read(WalkString path) {
    WalkString error = "";
    WalkString value = walk_rt_file_read_try_value(path, &error);
    if (error[0] != '\0') { walk_rt_panic(error); }
    return value;
}

FileReadResult walk_rt_file_try_read(WalkString path) {
    WalkString error = "";
    WalkString value = walk_rt_file_read_try_value(path, &error);
    if (error[0] != '\0') { return (FileReadResult){.ok = false, .value = "", .error = error}; }
    return (FileReadResult){.ok = true, .value = value, .error = ""};
}

static WALK_UNUSED WalkString walk_rt_file_write_try_error(WalkString path, WalkString text, const char *mode, const char *failure) {
    if (path == NULL || path[0] == '\0') { return "file path empty"; }
    if (text == NULL || !walk_rt_is_valid_utf8(text)) { return "file invalid utf-8"; }
    FILE *file = fopen(path, mode);
    if (file == NULL) { return failure; }
    size_t len = strlen(text);
    if (len > 0 && fwrite(text, 1, len, file) != len) {
        fclose(file);
        return failure;
    }
    if (fclose(file) != 0) { return failure; }
    return "";
}

void walk_rt_file_write(WalkString path, WalkString text) {
    WalkString error = walk_rt_file_write_try_error(path, text, "wb", "file write failed");
    if (error[0] != '\0') { walk_rt_panic(error); }
}

FileActionResult walk_rt_file_try_write(WalkString path, WalkString text) {
    WalkString error = walk_rt_file_write_try_error(path, text, "wb", "file write failed");
    WalkBool ok = error[0] == '\0';
    return (FileActionResult){.ok = ok, .value = ok, .error = error};
}

void walk_rt_file_append(WalkString path, WalkString text) {
    WalkString error = walk_rt_file_write_try_error(path, text, "ab", "file append failed");
    if (error[0] != '\0') { walk_rt_panic(error); }
}

FileActionResult walk_rt_file_try_append(WalkString path, WalkString text) {
    WalkString error = walk_rt_file_write_try_error(path, text, "ab", "file append failed");
    WalkBool ok = error[0] == '\0';
    return (FileActionResult){.ok = ok, .value = ok, .error = error};
}

WalkBool walk_rt_file_exists(WalkString path) {
    return walk_platform_file_exists(path);
}

static WALK_UNUSED void walk_rt_array_string_push_owned(WalkString **items, WalkSize *len, WalkSize *cap, WalkString item) {
    if (*len >= *cap) {
        WalkSize next_cap = *cap == 0 ? 4 : *cap * 2;
        if (next_cap <= *cap || (size_t)next_cap > ((size_t)-1) / sizeof(WalkString)) { walk_rt_panic("out of memory"); }
        WalkString *next_items = (WalkString *)realloc(*items, (size_t)next_cap * sizeof(WalkString));
        if (next_items == NULL) { walk_rt_panic("out of memory"); }
        *items = next_items;
        *cap = next_cap;
    }
    (*items)[*len] = item;
    *len += 1;
}

static WALK_UNUSED int walk_rt_string_pointer_compare(const void *left, const void *right) {
    const WalkString *a = (const WalkString *)left;
    const WalkString *b = (const WalkString *)right;
    return strcmp(*a, *b);
}

WalkArrayString walk_rt_dir_list(WalkString path) {
    if (path == NULL || path[0] == '\0') { walk_rt_panic("dir path empty"); }
    WalkString *items = NULL;
    WalkSize len = 0;
    WalkSize cap = 0;
#if defined(_WIN32)
    size_t path_len = strlen(path);
    WalkBool needs_sep = path_len > 0 && path[path_len - 1] != '/' && path[path_len - 1] != '\\';
    size_t pattern_len = path_len + (needs_sep ? 2 : 1) + 1;
    char *pattern = (char *)malloc(pattern_len);
    if (pattern == NULL) { walk_rt_panic("out of memory"); }
    if (needs_sep) { snprintf(pattern, pattern_len, "%s\\*", path); } else { snprintf(pattern, pattern_len, "%s*", path); }
    WIN32_FIND_DATAA data;
    HANDLE handle = FindFirstFileA(pattern, &data);
    free(pattern);
    if (handle == INVALID_HANDLE_VALUE) { walk_rt_panic("dir list failed"); }
    do {
        if (strcmp(data.cFileName, ".") != 0 && strcmp(data.cFileName, "..") != 0) { walk_rt_array_string_push_owned(&items, &len, &cap, walk_rt_copy_string(data.cFileName)); }
    } while (FindNextFileA(handle, &data) != 0);
    DWORD find_error = GetLastError();
    FindClose(handle);
    if (find_error != ERROR_NO_MORE_FILES) { walk_rt_panic("dir list failed"); }
#else
    DIR *dir = opendir(path);
    if (dir == NULL) { walk_rt_panic("dir list failed"); }
    for (;;) {
        errno = 0;
        struct dirent *entry = readdir(dir);
        if (entry == NULL) {
            if (errno != 0) { closedir(dir); walk_rt_panic("dir list failed"); }
            break;
        }
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { walk_rt_array_string_push_owned(&items, &len, &cap, walk_rt_copy_string(entry->d_name)); }
    }
    if (closedir(dir) != 0) { walk_rt_panic("dir list failed"); }
#endif
    if (len > 1) { qsort(items, (size_t)len, sizeof(WalkString), walk_rt_string_pointer_compare); }
    return (WalkArrayString){items, len};
}

void walk_rt_dir_make(WalkString path) {
    if (path == NULL || path[0] == '\0') { walk_rt_panic("dir path empty"); }
    if (!walk_platform_dir_make(path)) { walk_rt_panic("dir make failed"); }
}

void walk_rt_dir_delete(WalkString path) {
    if (path == NULL || path[0] == '\0') { walk_rt_panic("dir path empty"); }
    if (!walk_platform_dir_delete(path)) { walk_rt_panic("dir delete failed"); }
}

static WALK_UNUSED WalkBool walk_rt_is_path_separator(char ch) {
    return ch == '/' || ch == '\\';
}

WalkString walk_rt_path_join(WalkString left, WalkString right) {
    if (left == NULL) { left = ""; }
    if (right == NULL) { right = ""; }
    size_t left_len = strlen(left);
    size_t right_len = strlen(right);
    if (left_len == 0) { return walk_rt_copy_string(right); }
    if (right_len == 0) { return walk_rt_copy_string(left); }
    WalkBool add_sep = !walk_rt_is_path_separator(left[left_len - 1]) && !walk_rt_is_path_separator(right[0]);
    const char sep = walk_platform_path_separator();
    size_t extra = add_sep ? 1 : 0;
    if (left_len > ((size_t)-1) - right_len - extra - 1) { walk_rt_panic("out of memory"); }
    char *out = (char *)malloc(left_len + right_len + extra + 1);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    memcpy(out, left, left_len);
    size_t offset = left_len;
    if (add_sep) { out[offset++] = sep; }
    memcpy(out + offset, right, right_len + 1);
    return out;
}

WalkString walk_rt_path_base(WalkString path) {
    if (path == NULL) { return walk_rt_copy_string(""); }
    const char *base = path;
    for (const char *p = path; *p != '\0'; p++) { if (walk_rt_is_path_separator(*p)) { base = p + 1; } }
    return walk_rt_copy_string(base);
}

WalkString walk_rt_path_ext(WalkString path) {
    if (path == NULL) { return walk_rt_copy_string(""); }
    const char *dot = NULL;
    for (const char *p = path; *p != '\0'; p++) {
        if (walk_rt_is_path_separator(*p)) { dot = NULL; continue; }
        if (*p == '.') { dot = p; }
    }
    return walk_rt_copy_string(dot == NULL ? "" : dot);
}

WalkInt walk_rt_process_arg_count(void) {
    return walk_rt_host_argc > 0 ? (WalkInt)(walk_rt_host_argc - 1) : 0;
}

WalkArrayString walk_rt_process_args(void) {
    WalkInt count = walk_rt_process_arg_count();
    WalkString *items = (WalkString *)walk_rt_alloc_array(count, sizeof(WalkString));
    for (WalkInt i = 0; i < count; i++) { items[i] = walk_rt_host_argv[i + 1]; }
    return (WalkArrayString){items, count};
}

WalkString walk_rt_process_env(WalkString name) {
    if (name == NULL) { return NULL; }
    return getenv(name);
}

WalkString walk_rt_process_cwd(void) {
    WalkString cwd = walk_platform_cwd();
    if (cwd == NULL) { walk_rt_panic("cwd failed"); }
    return cwd;
}

void walk_rt_process_chdir(WalkString path) {
    if (path == NULL || path[0] == '\0') { walk_rt_panic("chdir path empty"); }
    if (!walk_platform_chdir(path)) { walk_rt_panic("chdir failed"); }
}

static WALK_UNUSED ProcessResult walk_rt_process_result(WalkBool ok, WalkInt status, WalkString stdout_text, WalkString stderr_text, WalkString error) {
    return (ProcessResult){.ok = ok, .status = status, .stdout = stdout_text == NULL ? "" : stdout_text, .stderr = stderr_text == NULL ? "" : stderr_text, .error = error == NULL ? "" : error};
}

static WALK_UNUSED WalkString walk_rt_process_temp_path(void) {
    WalkString path = walk_platform_temp_path();
    if (path == NULL) { walk_rt_panic("process temp failed"); }
    return path;
}

static WALK_UNUSED char **walk_rt_process_argv(WalkString command, WalkArrayString args) {
    if (args.len < 0) { walk_rt_panic("process args invalid"); }
    size_t count = (size_t)args.len;
    if (count > ((size_t)-1) / sizeof(char *) - 2) { walk_rt_panic("out of memory"); }
    char **argv = (char **)calloc(count + 2, sizeof(char *));
    if (argv == NULL) { walk_rt_panic("out of memory"); }
    argv[0] = (char *)command;
    for (WalkSize i = 0; i < args.len; i++) { argv[i + 1] = (char *)(args.items[i] == NULL ? "" : args.items[i]); }
    argv[count + 1] = NULL;
    return argv;
}

static WALK_UNUSED WalkString walk_rt_process_read_text(WalkString path, WalkString *error) {
    WalkString file_error = "";
    WalkString value = walk_rt_file_read_try_value(path, &file_error);
    if (file_error[0] != '\0') { *error = "process output read failed"; return ""; }
    return value;
}

ProcessResult walk_rt_process_run(WalkString command, WalkArrayString args) {
    if (command == NULL || command[0] == '\0') { return walk_rt_process_result(false, -1, "", "", "process command empty"); }
    WalkString stdout_path = walk_rt_process_temp_path();
    WalkString stderr_path = walk_rt_process_temp_path();
    char **argv = walk_rt_process_argv(command, args);
#if defined(_WIN32)
    int stdout_fd = _open(stdout_path, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE);
    int stderr_fd = _open(stderr_path, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (stdout_fd < 0 || stderr_fd < 0) {
        if (stdout_fd >= 0) { _close(stdout_fd); }
        if (stderr_fd >= 0) { _close(stderr_fd); }
        remove(stdout_path); remove(stderr_path); free(argv);
        return walk_rt_process_result(false, -1, "", "", "process spawn failed");
    }
    fflush(stdout); fflush(stderr);
    int old_stdout = _dup(1);
    int old_stderr = _dup(2);
    if (old_stdout < 0 || old_stderr < 0 || _dup2(stdout_fd, 1) != 0 || _dup2(stderr_fd, 2) != 0) {
        if (old_stdout >= 0) { _close(old_stdout); }
        if (old_stderr >= 0) { _close(old_stderr); }
        _close(stdout_fd); _close(stderr_fd); remove(stdout_path); remove(stderr_path); free(argv);
        return walk_rt_process_result(false, -1, "", "", "process spawn failed");
    }
    _close(stdout_fd); _close(stderr_fd);
    intptr_t status_code = _spawnvp(_P_WAIT, command, (const char * const *)argv);
    fflush(stdout); fflush(stderr);
    _dup2(old_stdout, 1); _dup2(old_stderr, 2);
    _close(old_stdout); _close(old_stderr);
    if (status_code == -1) {
        remove(stdout_path); remove(stderr_path); free(argv);
        return walk_rt_process_result(false, -1, "", "", "process spawn failed");
    }
    WalkInt status = (WalkInt)status_code;
#else
    pid_t pid = fork();
    if (pid < 0) { remove(stdout_path); remove(stderr_path); free(argv); return walk_rt_process_result(false, -1, "", "", "process spawn failed"); }
    if (pid == 0) {
        int stdout_fd = open(stdout_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        int stderr_fd = open(stderr_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (stdout_fd < 0 || stderr_fd < 0) { _exit(127); }
        if (dup2(stdout_fd, STDOUT_FILENO) < 0 || dup2(stderr_fd, STDERR_FILENO) < 0) { _exit(127); }
        close(stdout_fd); close(stderr_fd);
        execvp(command, argv);
        _exit(127);
    }
    int raw_status = 0;
    if (waitpid(pid, &raw_status, 0) < 0) { remove(stdout_path); remove(stderr_path); free(argv); return walk_rt_process_result(false, -1, "", "", "process wait failed"); }
    WalkInt status = -1;
    if (WIFEXITED(raw_status)) { status = (WalkInt)WEXITSTATUS(raw_status); }
    else if (WIFSIGNALED(raw_status)) { status = (WalkInt)(128 + WTERMSIG(raw_status)); }
#endif
    WalkString read_error = "";
    WalkString stdout_text = walk_rt_process_read_text(stdout_path, &read_error);
    WalkString stderr_text = walk_rt_process_read_text(stderr_path, &read_error);
    remove(stdout_path); remove(stderr_path); free(argv);
    if (read_error[0] != '\0') { return walk_rt_process_result(false, status, "", "", read_error); }
    WalkBool ok = status == 0;
    return walk_rt_process_result(ok, status, stdout_text, stderr_text, ok ? "" : "process exited non-zero");
}

ProcessOutputResult walk_rt_process_output(WalkString command, WalkArrayString args) {
    ProcessResult result = walk_rt_process_run(command, args);
    if (!result.ok) {
        WalkString error = result.error[0] != '\0' ? result.error : result.stderr;
        return (ProcessOutputResult){.ok = false, .value = result.stdout, .status = result.status, .error = error};
    }
    return (ProcessOutputResult){.ok = true, .value = result.stdout, .status = result.status, .error = ""};
}

ProcessResult walk_rt_process_run_shell(WalkString command) {
    if (command == NULL) { command = ""; }
#if defined(_WIN32)
    WalkString items[2] = {"/C", command};
    return walk_rt_process_run("cmd.exe", (WalkArrayString){items, 2});
#else
    WalkString items[2] = {"-c", command};
    return walk_rt_process_run("/bin/sh", (WalkArrayString){items, 2});
#endif
}

void walk_rt_process_exit(WalkInt code) {
    exit((int)code);
}

static WALK_UNUSED HttpResult walk_rt_http_result(WalkBool ok, WalkInt status, WalkString body, WalkString error) {
    return (HttpResult){.ok = ok, .status = status, .body = body == NULL ? "" : body, .error = error == NULL ? "" : error};
}

static WALK_UNUSED HttpResult walk_rt_http_parse(ProcessResult result) {
    if (!result.ok) { return walk_rt_http_result(false, result.status, result.stdout, result.error[0] != '\0' ? result.error : result.stderr); }
    const char *marker = "\n__WALK_HTTP_STATUS:";
    const char *last = NULL;
    const char *scan = result.stdout;
    while ((scan = strstr(scan, marker)) != NULL) { last = scan; scan += 1; }
    if (last == NULL) { return walk_rt_http_result(false, -1, result.stdout, "http status missing"); }
    const char *status_text = last + strlen(marker);
    errno = 0;
    char *end = NULL;
    long long status = strtoll(status_text, &end, 10);
    if (end == status_text || *end != '\0' || errno == ERANGE) { return walk_rt_http_result(false, -1, result.stdout, "http status invalid"); }
    size_t body_len = (size_t)(last - result.stdout);
    char *body = (char *)malloc(body_len + 1);
    if (body == NULL) { walk_rt_panic("out of memory"); }
    memcpy(body, result.stdout, body_len);
    body[body_len] = '\0';
    WalkBool ok = status >= 200 && status < 400;
    return walk_rt_http_result(ok, (WalkInt)status, body, ok ? "" : "http status");
}

HttpResult walk_rt_http_request(WalkString method, WalkString url, WalkString body) {
    if (method == NULL || method[0] == '\0') { return walk_rt_http_result(false, -1, "", "http method empty"); }
    if (url == NULL || url[0] == '\0') { return walk_rt_http_result(false, -1, "", "http url empty"); }
    if (body == NULL) { body = ""; }
    WalkString items[14];
    WalkSize len = 0;
    items[len++] = "--silent";
    items[len++] = "--show-error";
    items[len++] = "--location";
    items[len++] = "--max-time";
    items[len++] = "10";
    items[len++] = "--max-filesize";
    items[len++] = "1048576";
    items[len++] = "--write-out";
    items[len++] = "\n__WALK_HTTP_STATUS:%{http_code}";
    items[len++] = "-X";
    items[len++] = method;
    if (body[0] != '\0') {
        items[len++] = "--data-binary";
        items[len++] = body;
    }
    items[len++] = url;
    return walk_rt_http_parse(walk_rt_process_run("curl", (WalkArrayString){items, len}));
}

HttpResult walk_rt_http_get(WalkString url) {
    return walk_rt_http_request("GET", url, "");
}

HttpResult walk_rt_http_post(WalkString url, WalkString body) {
    return walk_rt_http_request("POST", url, body);
}

static WALK_UNUSED size_t walk_rt_html_escape_len(WalkString text) {
    if (text == NULL) { return 0; }
    size_t len = 0;
    for (const char *p = text; *p != '\0'; p++) {
        switch (*p) {
        case '&': len += 5; break;
        case '<': len += 4; break;
        case '>': len += 4; break;
        case '"': len += 6; break;
        case '\'': len += 5; break;
        default: len += 1; break;
        }
    }
    return len;
}

static WALK_UNUSED void walk_rt_html_append_escape(char **out, char ch) {
    const char *replacement = NULL;
    switch (ch) {
    case '&': replacement = "&amp;"; break;
    case '<': replacement = "&lt;"; break;
    case '>': replacement = "&gt;"; break;
    case '"': replacement = "&quot;"; break;
    case '\'': replacement = "&#39;"; break;
    }
    if (replacement != NULL) {
        size_t len = strlen(replacement);
        memcpy(*out, replacement, len);
        *out += len;
        return;
    }
    **out = ch;
    *out += 1;
}

WalkString walk_rt_html_escape(WalkString text) {
    size_t len = walk_rt_html_escape_len(text);
    char *out = (char *)malloc(len + 1);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    char *cursor = out;
    if (text != NULL) {
        for (const char *p = text; *p != '\0'; p++) { walk_rt_html_append_escape(&cursor, *p); }
    }
    *cursor = '\0';
    return out;
}

static WALK_UNUSED WalkString walk_rt_html_wrap(const char *tag, WalkString text) {
    WalkString escaped = walk_rt_html_escape(text);
    size_t tag_len = strlen(tag);
    size_t text_len = strlen(escaped);
    size_t total = tag_len + 2 + text_len + tag_len + 3;
    char *out = (char *)malloc(total + 1);
    if (out == NULL) { walk_rt_panic("out of memory"); }
    snprintf(out, total + 1, "<%s>%s</%s>", tag, escaped, tag);
    return out;
}

WalkString walk_rt_html_h1(WalkString text) {
    return walk_rt_html_wrap("h1", text);
}

WalkString walk_rt_html_p(WalkString text) {
    return walk_rt_html_wrap("p", text);
}

WalkString walk_rt_html_button(WalkString text) {
    return walk_rt_html_wrap("button", text);
}

static WALK_UNUSED WalkBool walk_rt_parse_starts_with_space(WalkString text) {
    return text[0] == ' ' || text[0] == '\t' || text[0] == '\n' || text[0] == '\r' || text[0] == '\f' || text[0] == '\v';
}

ParseIntResult walk_rt_parse_int(WalkString text) {
    if (text == NULL || text[0] == '\0' || walk_rt_parse_starts_with_space(text)) {
        return (ParseIntResult){.ok = false, .value = 0, .error = "invalid int"};
    }
    errno = 0;
    char *end = NULL;
    long long value = strtoll(text, &end, 10);
    if (end == text || *end != '\0') {
        return (ParseIntResult){.ok = false, .value = 0, .error = "invalid int"};
    }
    if (errno == ERANGE) {
        return (ParseIntResult){.ok = false, .value = 0, .error = "int out of range"};
    }
    return (ParseIntResult){.ok = true, .value = (WalkInt)value, .error = ""};
}

ParseFloatResult walk_rt_parse_float(WalkString text) {
    if (text == NULL || text[0] == '\0' || walk_rt_parse_starts_with_space(text)) {
        return (ParseFloatResult){.ok = false, .value = 0, .error = "invalid float"};
    }
    errno = 0;
    char *end = NULL;
    double value = strtod(text, &end);
    if (end == text || *end != '\0' || !isfinite(value)) {
        return (ParseFloatResult){.ok = false, .value = 0, .error = "invalid float"};
    }
    if (errno == ERANGE) {
        return (ParseFloatResult){.ok = false, .value = 0, .error = "float out of range"};
    }
    return (ParseFloatResult){.ok = true, .value = (WalkFloat)value, .error = ""};
}

ParseBoolResult walk_rt_parse_bool(WalkString text) {
    if (text != NULL && strcmp(text, "true") == 0) {
        return (ParseBoolResult){.ok = true, .value = true, .error = ""};
    }
    if (text != NULL && strcmp(text, "false") == 0) {
        return (ParseBoolResult){.ok = true, .value = false, .error = ""};
    }
    return (ParseBoolResult){.ok = false, .value = false, .error = "invalid bool"};
}

typedef struct { char *items; size_t len; size_t cap; } WalkJsonBuilder;

static WALK_UNUSED JsonResult walk_rt_json_result(WalkBool ok, WalkString value, WalkString error) {
    return (JsonResult){.ok = ok, .value = value == NULL ? "" : value, .error = error == NULL ? "" : error};
}

static WALK_UNUSED void walk_rt_json_builder_init(WalkJsonBuilder *builder) {
    builder->items = NULL;
    builder->len = 0;
    builder->cap = 0;
}

static WALK_UNUSED void walk_rt_json_builder_append_char(WalkJsonBuilder *builder, char ch) {
    if (builder->len + 1 >= builder->cap) {
        size_t next_cap = builder->cap == 0 ? 64 : builder->cap * 2;
        if (next_cap <= builder->cap) { walk_rt_panic("out of memory"); }
        char *next_items = (char *)realloc(builder->items, next_cap);
        if (next_items == NULL) { walk_rt_panic("out of memory"); }
        builder->items = next_items;
        builder->cap = next_cap;
    }
    builder->items[builder->len++] = ch;
}

static WALK_UNUSED void walk_rt_json_builder_append_range(WalkJsonBuilder *builder, const char *start, size_t len) {
    for (size_t i = 0; i < len; i++) { walk_rt_json_builder_append_char(builder, start[i]); }
}

static WALK_UNUSED void walk_rt_json_builder_append_cstring(WalkJsonBuilder *builder, const char *text) {
    walk_rt_json_builder_append_range(builder, text, strlen(text));
}

static WALK_UNUSED WalkString walk_rt_json_builder_finish(WalkJsonBuilder *builder) {
    walk_rt_json_builder_append_char(builder, '\0');
    return builder->items;
}

static WALK_UNUSED void walk_rt_json_skip_ws(const char **cursor) {
    while (**cursor == ' ' || **cursor == '\t' || **cursor == '\n' || **cursor == '\r') { *cursor += 1; }
}

static WALK_UNUSED WalkBool walk_rt_json_is_hex(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static WALK_UNUSED WalkBool walk_rt_json_parse_value(const char **cursor, WalkJsonBuilder *out);

static WALK_UNUSED WalkBool walk_rt_json_parse_string(const char **cursor, WalkJsonBuilder *out) {
    if (**cursor != '"') { return false; }
    const char *start = *cursor;
    *cursor += 1;
    while (**cursor != '\0') {
        unsigned char ch = (unsigned char)**cursor;
        if (ch < 0x20) { return false; }
        if (ch == '"') {
            *cursor += 1;
            walk_rt_json_builder_append_range(out, start, (size_t)(*cursor - start));
            return true;
        }
        if (ch == '\\') {
            *cursor += 1;
            char esc = **cursor;
            if (esc == '\0') { return false; }
            if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                *cursor += 1;
                continue;
            }
            if (esc == 'u') {
                for (int i = 0; i < 4; i++) {
                    *cursor += 1;
                    if (!walk_rt_json_is_hex(**cursor)) { return false; }
                }
                *cursor += 1;
                continue;
            }
            return false;
        }
        *cursor += 1;
    }
    return false;
}

static WALK_UNUSED WalkBool walk_rt_json_parse_number(const char **cursor, WalkJsonBuilder *out) {
    const char *start = *cursor;
    if (**cursor == '-') { *cursor += 1; }
    if (**cursor == '0') {
        *cursor += 1;
    } else if (**cursor >= '1' && **cursor <= '9') {
        while (**cursor >= '0' && **cursor <= '9') { *cursor += 1; }
    } else {
        return false;
    }
    if (**cursor == '.') {
        *cursor += 1;
        if (!(**cursor >= '0' && **cursor <= '9')) { return false; }
        while (**cursor >= '0' && **cursor <= '9') { *cursor += 1; }
    }
    if (**cursor == 'e' || **cursor == 'E') {
        *cursor += 1;
        if (**cursor == '+' || **cursor == '-') { *cursor += 1; }
        if (!(**cursor >= '0' && **cursor <= '9')) { return false; }
        while (**cursor >= '0' && **cursor <= '9') { *cursor += 1; }
    }
    walk_rt_json_builder_append_range(out, start, (size_t)(*cursor - start));
    return true;
}

static WALK_UNUSED WalkBool walk_rt_json_parse_literal(const char **cursor, WalkJsonBuilder *out, const char *literal) {
    size_t len = strlen(literal);
    if (strncmp(*cursor, literal, len) != 0) { return false; }
    walk_rt_json_builder_append_cstring(out, literal);
    *cursor += len;
    return true;
}

static WALK_UNUSED WalkBool walk_rt_json_parse_array(const char **cursor, WalkJsonBuilder *out) {
    if (**cursor != '[') { return false; }
    walk_rt_json_builder_append_char(out, '[');
    *cursor += 1;
    walk_rt_json_skip_ws(cursor);
    if (**cursor == ']') { walk_rt_json_builder_append_char(out, ']'); *cursor += 1; return true; }
    for (;;) {
        if (!walk_rt_json_parse_value(cursor, out)) { return false; }
        walk_rt_json_skip_ws(cursor);
        if (**cursor == ',') { walk_rt_json_builder_append_char(out, ','); *cursor += 1; walk_rt_json_skip_ws(cursor); continue; }
        if (**cursor == ']') { walk_rt_json_builder_append_char(out, ']'); *cursor += 1; return true; }
        return false;
    }
}

static WALK_UNUSED WalkBool walk_rt_json_parse_object(const char **cursor, WalkJsonBuilder *out) {
    if (**cursor != '{') { return false; }
    walk_rt_json_builder_append_char(out, '{');
    *cursor += 1;
    walk_rt_json_skip_ws(cursor);
    if (**cursor == '}') { walk_rt_json_builder_append_char(out, '}'); *cursor += 1; return true; }
    for (;;) {
        if (!walk_rt_json_parse_string(cursor, out)) { return false; }
        walk_rt_json_skip_ws(cursor);
        if (**cursor != ':') { return false; }
        walk_rt_json_builder_append_char(out, ':');
        *cursor += 1;
        if (!walk_rt_json_parse_value(cursor, out)) { return false; }
        walk_rt_json_skip_ws(cursor);
        if (**cursor == ',') { walk_rt_json_builder_append_char(out, ','); *cursor += 1; walk_rt_json_skip_ws(cursor); continue; }
        if (**cursor == '}') { walk_rt_json_builder_append_char(out, '}'); *cursor += 1; return true; }
        return false;
    }
}

static WALK_UNUSED WalkBool walk_rt_json_parse_value(const char **cursor, WalkJsonBuilder *out) {
    walk_rt_json_skip_ws(cursor);
    switch (**cursor) {
    case '{': return walk_rt_json_parse_object(cursor, out);
    case '[': return walk_rt_json_parse_array(cursor, out);
    case '"': return walk_rt_json_parse_string(cursor, out);
    case 't': return walk_rt_json_parse_literal(cursor, out, "true");
    case 'f': return walk_rt_json_parse_literal(cursor, out, "false");
    case 'n': return walk_rt_json_parse_literal(cursor, out, "null");
    default:
        if (**cursor == '-' || (**cursor >= '0' && **cursor <= '9')) { return walk_rt_json_parse_number(cursor, out); }
        return false;
    }
}

JsonResult walk_rt_json_parse(WalkString text) {
    if (text == NULL) { return walk_rt_json_result(false, "", "invalid json"); }
    const char *cursor = text;
    WalkJsonBuilder builder;
    walk_rt_json_builder_init(&builder);
    walk_rt_json_skip_ws(&cursor);
    if (!walk_rt_json_parse_value(&cursor, &builder)) { return walk_rt_json_result(false, "", "invalid json"); }
    walk_rt_json_skip_ws(&cursor);
    if (*cursor != '\0') { return walk_rt_json_result(false, "", "invalid json"); }
    return walk_rt_json_result(true, walk_rt_json_builder_finish(&builder), "");
}

WalkString walk_rt_json_stringify(WalkString text) {
    if (text == NULL) { text = ""; }
    const char *hex = "0123456789abcdef";
    WalkJsonBuilder builder;
    walk_rt_json_builder_init(&builder);
    walk_rt_json_builder_append_char(&builder, '"');
    for (const unsigned char *p = (const unsigned char *)text; *p != '\0'; p++) {
        unsigned char ch = *p;
        if (ch == '"') { walk_rt_json_builder_append_cstring(&builder, "\\\""); }
        else if (ch == '\\') { walk_rt_json_builder_append_cstring(&builder, "\\\\"); }
        else if (ch == 8) { walk_rt_json_builder_append_cstring(&builder, "\\b"); }
        else if (ch == 12) { walk_rt_json_builder_append_cstring(&builder, "\\f"); }
        else if (ch == '\n') { walk_rt_json_builder_append_cstring(&builder, "\\n"); }
        else if (ch == '\r') { walk_rt_json_builder_append_cstring(&builder, "\\r"); }
        else if (ch == '\t') { walk_rt_json_builder_append_cstring(&builder, "\\t"); }
        else if (ch < 0x20) {
            walk_rt_json_builder_append_cstring(&builder, "\\u00");
            walk_rt_json_builder_append_char(&builder, hex[(ch >> 4) & 0xF]);
            walk_rt_json_builder_append_char(&builder, hex[ch & 0xF]);
        } else {
            walk_rt_json_builder_append_char(&builder, (char)ch);
        }
    }
    walk_rt_json_builder_append_char(&builder, '"');
    return walk_rt_json_builder_finish(&builder);
}

JsonResult walk_rt_json_read(WalkString path) {
    WalkString error = "";
    WalkString text = walk_rt_file_read_try_value(path, &error);
    if (error[0] != '\0') { return walk_rt_json_result(false, "", error); }
    return walk_rt_json_parse(text);
}

void walk_rt_json_write(WalkString path, WalkString text) {
    JsonResult parsed = walk_rt_json_parse(text);
    if (!parsed.ok) { walk_rt_panic(parsed.error); }
    WalkString error = walk_rt_file_write_try_error(path, parsed.value, "wb", "file write failed");
    if (error[0] != '\0') { walk_rt_panic(error); }
}
