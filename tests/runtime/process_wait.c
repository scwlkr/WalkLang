/* Inject wait errors at the real process.run seam, without timing-dependent signals. */
#include "../../runtime/walk_runtime.h"
#include "../../runtime/platform/walk_platform.h"
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t injected_waitpid(pid_t pid, int *status, int options);
static WalkString tracked_temp_path(void);
#define waitpid injected_waitpid
#define walk_platform_temp_path tracked_temp_path
#include "../../runtime/walk_runtime.c"
#undef walk_platform_temp_path
#undef waitpid

static int interruptions;
static int terminal_error;
static int wait_calls;
static int failures;
static pid_t child_pid;
static WalkString capture_paths[2];
static int capture_count;

static void check(WalkBool condition, const char *name) {
    if (!condition) { fprintf(stderr, "FAIL process wait: %s\n", name); failures++; }
}

static WalkString tracked_temp_path(void) {
    WalkString path = walk_platform_temp_path();
    if (capture_count < 2) { capture_paths[capture_count++] = path; }
    return path;
}

static pid_t injected_waitpid(pid_t pid, int *status, int options) {
    check(pid > 0 && options == 0, "wait targets its own child without special options");
    if (wait_calls > 0) { check(pid == child_pid, "retry targets the same child"); }
    child_pid = pid;
    wait_calls++;
    if (interruptions > 0) { interruptions--; errno = EINTR; return -1; }
    if (terminal_error != 0) { errno = terminal_error; return -1; }
    return waitpid(pid, status, options);
}

static void run_case(int interrupted, int error, int exit_status) {
    interruptions = interrupted;
    terminal_error = error;
    wait_calls = 0;
    child_pid = -1;
    capture_count = 0;
    char command[128];
    snprintf(command, sizeof(command), "printf child-out; printf child-err >&2; exit %d", exit_status);
    WalkString args[] = {"-c", command};
    ProcessResult result = walk_rt_process_run("/bin/sh", (WalkArrayString){args, 2});
    check(wait_calls == (error == 0 ? interrupted + 1 : 1), "retry count");
    if (error == 0) {
        check(result.ok == (exit_status == 0), "success flag");
        check(result.status == exit_status, "actual child exit status");
        check(strcmp(result.stdout, "child-out") == 0, "stdout preserved");
        check(strcmp(result.stderr, "child-err") == 0, "stderr preserved");
        check(strcmp(result.error, exit_status == 0 ? "" : "process exited non-zero") == 0, "result error");
    } else {
        check(!result.ok && result.status == -1, "terminal wait error result");
        check(strcmp(result.error, "process wait failed") == 0, "terminal wait error retained");
        check(strcmp(result.stdout, "") == 0 && strcmp(result.stderr, "") == 0, "terminal error output");
    }

    /* Reap the real child even when testing the original broken implementation
       or a simulated fatal error, then clean only our recorded capture paths. */
    int status;
    errno = 0;
    pid_t reaped;
    do { reaped = waitpid(child_pid, &status, 0); } while (reaped < 0 && errno == EINTR);
    if (error == 0) { check(reaped == -1 && errno == ECHILD, "runtime reaped child"); }
    for (int i = 0; i < capture_count; i++) {
        if (error == 0) { check(access(capture_paths[i], F_OK) != 0, "runtime removed captures after wait"); }
        remove(capture_paths[i]);
        free((void *)capture_paths[i]);
    }
}

int main(void) {
    run_case(0, 0, 0);
    run_case(1, 0, 0);
    run_case(3, 0, 7);
    run_case(0, EINVAL, 0);
    if (failures != 0) { return 1; }
    puts("process.run interrupted wait tests passed");
    return 0;
}
