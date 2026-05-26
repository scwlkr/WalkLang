#include "walk_platform.h"

#include <conio.h>
#include <direct.h>
#include <io.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>

#if !defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static WalkString walk_platform_copy_string(const char *value) {
    if (value == NULL) { value = ""; }
    size_t len = strlen(value);
    char *out = (char *)malloc(len + 1);
    if (out == NULL) { return NULL; }
    memcpy(out, value, len + 1);
    return out;
}

WalkBool walk_platform_stdout_is_tty(void) {
    return _isatty(_fileno(stdout)) != 0;
}

WalkBool walk_platform_stdin_is_tty(void) {
    return _isatty(_fileno(stdin)) != 0;
}

WalkBool walk_platform_enable_ansi(void) {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) { return false; }
    DWORD mode = 0;
    if (!GetConsoleMode(handle, &mode)) { return false; }
    if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
        if (!SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) { return false; }
    }
    return true;
}

WalkInt walk_platform_terminal_width(void) {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (walk_platform_stdout_is_tty() && handle != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(handle, &info)) {
        return (WalkInt)(info.srWindow.Right - info.srWindow.Left + 1);
    }
    return 0;
}

WalkInt walk_platform_terminal_height(void) {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (walk_platform_stdout_is_tty() && handle != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(handle, &info)) {
        return (WalkInt)(info.srWindow.Bottom - info.srWindow.Top + 1);
    }
    return 0;
}

WalkBool walk_platform_read_key(char *out, const char **error) {
    int ch = _getch();
    if (ch == 0 || ch == 224) { (void)_getch(); *error = "terminal key unsupported"; return false; }
    if (ch < 0) { *error = "terminal read failed"; return false; }
    if (ch == 0) { *error = "terminal key unsupported"; return false; }
    *out = (char)ch;
    *error = "";
    return true;
}

WalkBool walk_platform_file_exists(WalkString path) {
    struct _stat info;
    return path != NULL && path[0] != '\0' && _stat(path, &info) == 0;
}

WalkBool walk_platform_dir_make(WalkString path) {
    return _mkdir(path) == 0;
}

WalkBool walk_platform_dir_delete(WalkString path) {
    return _rmdir(path) == 0;
}

char walk_platform_path_separator(void) {
    return '\\';
}

WalkString walk_platform_cwd(void) {
    size_t cap = 256;
    for (;;) {
        if (cap > (size_t)INT_MAX) { return NULL; }
        char *buffer = (char *)malloc(cap);
        if (buffer == NULL) { return NULL; }
        if (_getcwd(buffer, (int)cap) != NULL) { return buffer; }
        free(buffer);
        if (cap > ((size_t)-1) / 2) { return NULL; }
        cap *= 2;
    }
}

WalkBool walk_platform_chdir(WalkString path) {
    return _chdir(path) == 0;
}

WalkString walk_platform_temp_path(void) {
    char buffer[L_tmpnam];
    if (tmpnam(buffer) == NULL) { return NULL; }
    return walk_platform_copy_string(buffer);
}
