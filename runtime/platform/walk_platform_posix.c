#include "walk_platform.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

static WalkString walk_platform_copy_string(const char *value) {
    if (value == NULL) { value = ""; }
    size_t len = strlen(value);
    char *out = (char *)malloc(len + 1);
    if (out == NULL) { return NULL; }
    memcpy(out, value, len + 1);
    return out;
}

WalkBool walk_platform_stdout_is_tty(void) {
    return isatty(fileno(stdout)) != 0;
}

WalkBool walk_platform_stdin_is_tty(void) {
    return isatty(fileno(stdin)) != 0;
}

WalkBool walk_platform_enable_ansi(void) {
    return true;
}

WalkInt walk_platform_terminal_width(void) {
    struct winsize size;
    if (walk_platform_stdout_is_tty() && ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0 && size.ws_col > 0) {
        return (WalkInt)size.ws_col;
    }
    return 0;
}

WalkInt walk_platform_terminal_height(void) {
    struct winsize size;
    if (walk_platform_stdout_is_tty() && ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0 && size.ws_row > 0) {
        return (WalkInt)size.ws_row;
    }
    return 0;
}

WalkBool walk_platform_read_key(char *out, const char **error) {
    struct termios old_mode;
    if (tcgetattr(fileno(stdin), &old_mode) != 0) { *error = "terminal read failed"; return false; }
    struct termios raw = old_mode;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(fileno(stdin), TCSANOW, &raw) != 0) { *error = "terminal raw mode failed"; return false; }
    char ch = '\0';
    ssize_t count = read(fileno(stdin), &ch, 1);
    if (tcsetattr(fileno(stdin), TCSANOW, &old_mode) != 0) { *error = "terminal restore failed"; return false; }
    if (count < 0) { *error = "terminal read failed"; return false; }
    if (count == 0) { *error = "eof"; return false; }
    if (ch == '\0') { *error = "terminal key unsupported"; return false; }
    *out = ch;
    *error = "";
    return true;
}

WalkBool walk_platform_file_exists(WalkString path) {
    struct stat info;
    return path != NULL && path[0] != '\0' && stat(path, &info) == 0;
}

WalkBool walk_platform_dir_make(WalkString path) {
    return mkdir(path, 0777) == 0;
}

WalkBool walk_platform_dir_delete(WalkString path) {
    return rmdir(path) == 0;
}

char walk_platform_path_separator(void) {
    return '/';
}

WalkString walk_platform_cwd(void) {
    size_t cap = 256;
    for (;;) {
        char *buffer = (char *)malloc(cap);
        if (buffer == NULL) { return NULL; }
        errno = 0;
        if (getcwd(buffer, cap) != NULL) { return buffer; }
        free(buffer);
        if (errno != ERANGE || cap > ((size_t)-1) / 2) { return NULL; }
        cap *= 2;
    }
}

WalkBool walk_platform_chdir(WalkString path) {
    return chdir(path) == 0;
}

WalkString walk_platform_temp_path(void) {
    char path[] = "/tmp/walklang-process-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) { return NULL; }
    close(fd);
    return walk_platform_copy_string(path);
}
