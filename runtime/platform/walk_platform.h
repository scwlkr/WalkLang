#ifndef WALK_PLATFORM_H
#define WALK_PLATFORM_H

#include "../walk_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

WalkBool walk_platform_stdout_is_tty(void);
WalkBool walk_platform_stdin_is_tty(void);
WalkBool walk_platform_enable_ansi(void);
WalkInt walk_platform_terminal_width(void);
WalkInt walk_platform_terminal_height(void);
WalkBool walk_platform_read_key(char *out, const char **error);
WalkBool walk_platform_file_exists(WalkString path);
WalkBool walk_platform_dir_make(WalkString path);
WalkBool walk_platform_dir_delete(WalkString path);
char walk_platform_path_separator(void);
WalkString walk_platform_cwd(void);
WalkBool walk_platform_chdir(WalkString path);
WalkString walk_platform_temp_path(void);

#ifdef __cplusplus
}
#endif

#endif
