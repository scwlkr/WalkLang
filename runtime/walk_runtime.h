#ifndef WALK_RUNTIME_H
#define WALK_RUNTIME_H

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define WALK_UNUSED __attribute__((unused))
#else
#define WALK_UNUSED
#endif

/* walk runtime: no user pointers; array storage is runtime-owned for the process lifetime. */
typedef long long WalkInt;
typedef double WalkFloat;
typedef bool WalkBool;
typedef const char *WalkString;
typedef long long WalkSize;

typedef struct { WalkInt *items; WalkSize len; } WalkArrayInt;
typedef struct { WalkFloat *items; WalkSize len; } WalkArrayFloat;
typedef struct { WalkBool *items; WalkSize len; } WalkArrayBool;
typedef struct { WalkString *items; WalkSize len; } WalkArrayString;
typedef struct {
    WalkString key;
    WalkArrayString value;
} WalkMapStringArrayStringEntry;
typedef struct { WalkMapStringArrayStringEntry *entries; WalkSize len; } WalkMapStringArrayString;

typedef struct {
    WalkBool ok;
    WalkBool value;
    WalkString error;
} FileActionResult;
typedef struct { FileActionResult *items; WalkSize len; } WalkArrayFileActionResult;

typedef struct {
    WalkBool ok;
    WalkString value;
    WalkString error;
} FileReadResult;
typedef struct { FileReadResult *items; WalkSize len; } WalkArrayFileReadResult;

typedef struct {
    WalkBool ok;
    WalkInt status;
    WalkString body;
    WalkString error;
} HttpResult;
typedef struct { HttpResult *items; WalkSize len; } WalkArrayHttpResult;

typedef struct {
    WalkBool ok;
    WalkString value;
    WalkString error;
} IOReadResult;
typedef struct { IOReadResult *items; WalkSize len; } WalkArrayIOReadResult;

typedef struct {
    WalkBool ok;
    WalkString value;
    WalkString error;
} JsonResult;
typedef struct { JsonResult *items; WalkSize len; } WalkArrayJsonResult;

typedef struct {
    WalkBool ok;
    WalkBool value;
    WalkString error;
} ParseBoolResult;
typedef struct { ParseBoolResult *items; WalkSize len; } WalkArrayParseBoolResult;

typedef struct {
    WalkBool ok;
    WalkFloat value;
    WalkString error;
} ParseFloatResult;
typedef struct { ParseFloatResult *items; WalkSize len; } WalkArrayParseFloatResult;

typedef struct {
    WalkBool ok;
    WalkInt value;
    WalkString error;
} ParseIntResult;
typedef struct { ParseIntResult *items; WalkSize len; } WalkArrayParseIntResult;

typedef struct {
    WalkBool ok;
    WalkString value;
    WalkInt status;
    WalkString error;
} ProcessOutputResult;
typedef struct { ProcessOutputResult *items; WalkSize len; } WalkArrayProcessOutputResult;

typedef struct {
    WalkBool ok;
    WalkInt status;
    WalkString stdout;
    WalkString stderr;
    WalkString error;
} ProcessResult;
typedef struct { ProcessResult *items; WalkSize len; } WalkArrayProcessResult;

void walk_rt_init(int argc, char **argv);
void walk_rt_panic(const char *message);
void *walk_rt_alloc_array(WalkSize len, size_t item_size);

WalkInt walk_rt_random_int(WalkInt min, WalkInt max);
WalkFloat walk_rt_random_float(WalkFloat min, WalkFloat max);
WalkInt walk_rt_array_choice_int(WalkArrayInt array);
WalkFloat walk_rt_array_choice_float(WalkArrayFloat array);
WalkBool walk_rt_array_choice_bool(WalkArrayBool array);
WalkString walk_rt_array_choice_string(WalkArrayString array);

WalkInt walk_rt_string_len(WalkString value);
WalkString walk_rt_string_at(WalkString value, WalkInt index);
WalkBool walk_rt_string_contains(WalkString text, WalkString item);
WalkString walk_rt_string_concat(WalkString left, WalkString right);
WalkString walk_rt_string_lower(WalkString text);
WalkArrayString walk_rt_string_split(WalkString text, WalkString sep);
WalkString walk_rt_string_replace(WalkString text, WalkString from, WalkString to);
WalkString walk_rt_format_int(WalkInt value);
WalkString walk_rt_format_float(WalkFloat value);
WalkString walk_rt_format_bool(WalkBool value);
WalkString walk_rt_format_string(WalkString value);
WalkString walk_rt_input_line(WalkString prompt);

void walk_rt_print_int(WalkInt value);
void walk_rt_print_float(WalkFloat value);
void walk_rt_print_bool(WalkBool value);
void walk_rt_print_string(WalkString value);

WalkArrayInt walk_rt_array_push_int(WalkArrayInt array, WalkInt item);
WalkArrayFloat walk_rt_array_push_float(WalkArrayFloat array, WalkFloat item);
WalkArrayBool walk_rt_array_push_bool(WalkArrayBool array, WalkBool item);
WalkArrayString walk_rt_array_push_string(WalkArrayString array, WalkString item);
WalkBool walk_rt_array_contains_int(WalkArrayInt array, WalkInt item);
WalkBool walk_rt_array_contains_float(WalkArrayFloat array, WalkFloat item);
WalkBool walk_rt_array_contains_bool(WalkArrayBool array, WalkBool item);
WalkBool walk_rt_array_contains_string(WalkArrayString array, WalkString item);
WalkMapStringArrayString walk_rt_map_string_array_string_empty(void);
WalkBool walk_rt_map_string_array_string_has(WalkMapStringArrayString table, WalkString key);
WalkArrayString walk_rt_map_string_array_string_get(WalkMapStringArrayString table, WalkString key);
WalkMapStringArrayString walk_rt_map_string_array_string_set(WalkMapStringArrayString table, WalkString key, WalkArrayString value);
WalkArrayString walk_rt_map_string_array_string_keys(WalkMapStringArrayString table);
WalkMapStringArrayString walk_rt_map_string_array_string_push(WalkMapStringArrayString table, WalkString key, WalkString value);

void walk_rt_io_write(WalkString value);
void walk_rt_io_write_line(WalkString value);
void walk_rt_io_error_line(WalkString value);
IOReadResult walk_rt_io_read_line(void);
IOReadResult walk_rt_io_read_all(void);

WalkString walk_rt_file_read(WalkString path);
FileReadResult walk_rt_file_try_read(WalkString path);
void walk_rt_file_write(WalkString path, WalkString text);
FileActionResult walk_rt_file_try_write(WalkString path, WalkString text);
void walk_rt_file_append(WalkString path, WalkString text);
FileActionResult walk_rt_file_try_append(WalkString path, WalkString text);
WalkBool walk_rt_file_exists(WalkString path);
WalkArrayString walk_rt_dir_list(WalkString path);
void walk_rt_dir_make(WalkString path);
void walk_rt_dir_delete(WalkString path);
WalkString walk_rt_path_join(WalkString left, WalkString right);
WalkString walk_rt_path_base(WalkString path);
WalkString walk_rt_path_ext(WalkString path);

WalkArrayString walk_rt_process_args(void);
WalkInt walk_rt_process_arg_count(void);
WalkString walk_rt_process_env(WalkString name);
WalkString walk_rt_process_cwd(void);
void walk_rt_process_chdir(WalkString path);
ProcessResult walk_rt_process_run(WalkString command, WalkArrayString args);
ProcessOutputResult walk_rt_process_output(WalkString command, WalkArrayString args);
ProcessResult walk_rt_process_run_shell(WalkString command);
void walk_rt_process_exit(WalkInt code);

ParseIntResult walk_rt_parse_int(WalkString text);
ParseFloatResult walk_rt_parse_float(WalkString text);
ParseBoolResult walk_rt_parse_bool(WalkString text);
JsonResult walk_rt_json_parse(WalkString text);
WalkString walk_rt_json_stringify(WalkString text);
JsonResult walk_rt_json_read(WalkString path);
void walk_rt_json_write(WalkString path, WalkString text);

WalkBool walk_rt_term_is_tty(void);
void walk_rt_term_color(WalkString name);
void walk_rt_term_background(WalkString name);
void walk_rt_term_style(WalkString name);
void walk_rt_term_reset(void);
void walk_rt_term_clear(void);
void walk_rt_term_move(WalkInt column, WalkInt row);
WalkInt walk_rt_term_width(void);
WalkInt walk_rt_term_height(void);
IOReadResult walk_rt_term_read_key(void);

HttpResult walk_rt_http_get(WalkString url);
HttpResult walk_rt_http_post(WalkString url, WalkString body);
HttpResult walk_rt_http_request(WalkString method, WalkString url, WalkString body);
WalkString walk_rt_html_escape(WalkString text);
WalkString walk_rt_html_h1(WalkString text);
WalkString walk_rt_html_p(WalkString text);
WalkString walk_rt_html_button(WalkString text);

#ifdef __cplusplus
}
#endif

#endif
