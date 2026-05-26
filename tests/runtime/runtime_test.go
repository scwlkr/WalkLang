package runtime_test

import (
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"testing"

	"walklang/internal/runtimebuild"
)

func TestRuntimeCoversCoreHelpers(t *testing.T) {
	exePath := buildRuntimeProgram(t, strings.Join([]string{
		`#include "walk_runtime.h"`,
		`#include <stdlib.h>`,
		`#include <string.h>`,
		``,
		`static void check(int ok, const char *message) {`,
		`    if (!ok) { fprintf(stderr, "%s\n", message); exit(2); }`,
		`}`,
		``,
		`int main(int argc, char **argv) {`,
		`    walk_rt_init(argc, argv);`,
		`    check(argc == 2, "expected scratch directory argument");`,
		``,
		`    WalkInt *allocated = (WalkInt *)walk_rt_alloc_array(3, sizeof(WalkInt));`,
		`    check(allocated != NULL, "allocation failed");`,
		`    allocated[0] = 7;`,
		`    allocated[2] = 11;`,
		`    check(allocated[0] == 7 && allocated[1] == 0 && allocated[2] == 11, "allocation was not zeroed");`,
		``,
		`    WalkString joined_text = walk_rt_string_concat("walk", "lang");`,
		`    check(strcmp(joined_text, "walklang") == 0, "string concat failed");`,
		`    check(walk_rt_string_len("abcd") == 4, "string len failed");`,
		`    check(strcmp(walk_rt_string_at("abcd", 2), "c") == 0, "string at failed");`,
		`    check(walk_rt_string_contains("walklang", "lang"), "string contains failed");`,
		``,
		`    WalkArrayInt nums = {NULL, 0};`,
		`    nums = walk_rt_array_push_int(nums, 2);`,
		`    nums = walk_rt_array_push_int(nums, 5);`,
		`    check(nums.len == 2 && nums.items[0] == 2 && nums.items[1] == 5, "array push failed");`,
		`    check(walk_rt_array_contains_int(nums, 5), "array contains failed");`,
		``,
		`    WalkString file_path = walk_rt_path_join(argv[1], "runtime.txt");`,
		`    walk_rt_file_write(file_path, "alpha");`,
		`    walk_rt_file_append(file_path, " beta");`,
		`    check(walk_rt_file_exists(file_path), "file exists failed");`,
		`    check(strcmp(walk_rt_file_read(file_path), "alpha beta") == 0, "file read/write failed");`,
		``,
		`    FileReadResult read_result = walk_rt_file_try_read(file_path);`,
		`    check(read_result.ok && strcmp(read_result.value, "alpha beta") == 0, "file try_read failed");`,
		``,
		`    WalkString dir_path = walk_rt_path_join(argv[1], "runtime-dir");`,
		`    walk_rt_dir_make(dir_path);`,
		`    check(walk_rt_file_exists(dir_path), "dir make failed");`,
		`    walk_rt_dir_delete(dir_path);`,
		`    check(!walk_rt_file_exists(dir_path), "dir delete failed");`,
		``,
		`    WalkString cwd = walk_rt_process_cwd();`,
		`    check(cwd != NULL && cwd[0] != '\0', "cwd failed");`,
		`    ProcessResult shell = walk_rt_process_run_shell("echo process-ok");`,
		`    check(shell.ok && shell.status == 0 && strstr(shell.stdout, "process-ok") != NULL, "process shell failed");`,
		``,
		`    check(walk_rt_term_width() == 123, "terminal width fallback failed");`,
		`    check(walk_rt_term_height() == 45, "terminal height fallback failed");`,
		`    IOReadResult key = walk_rt_term_read_key();`,
		`    check(!key.ok && strcmp(key.error, "terminal not interactive") == 0, "terminal read-key error failed");`,
		``,
		`    printf("ok\n");`,
		`    return 0;`,
		`}`,
	}, "\n"))

	command := exec.Command(exePath, t.TempDir())
	command.Env = append(os.Environ(), "COLUMNS=123", "LINES=45", "NO_COLOR=1")
	output, err := command.CombinedOutput()
	if err != nil {
		t.Fatalf("runtime helper program failed: %v\n%s", err, string(output))
	}
	if got, want := string(output), "ok\n"; got != want {
		t.Fatalf("runtime helper output mismatch: want %q got %q", want, got)
	}
}

func TestRuntimePanicPrintsStableErrorMessage(t *testing.T) {
	exePath := buildRuntimeProgram(t, strings.Join([]string{
		`#include "walk_runtime.h"`,
		``,
		`int main(void) {`,
		`    walk_rt_panic("phase2 panic");`,
		`    return 0;`,
		`}`,
	}, "\n"))

	output, err := exec.Command(exePath).CombinedOutput()
	if err == nil {
		t.Fatalf("expected panic program to fail, got output %q", string(output))
	}
	if got, want := string(output), "walk runtime error: phase2 panic\n"; got != want {
		t.Fatalf("panic output mismatch: want %q got %q", want, got)
	}
}

func buildRuntimeProgram(t *testing.T, source string) string {
	t.Helper()
	if _, err := exec.LookPath("cc"); err != nil {
		t.Skip("cc is not available")
	}
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "runtime_test.c")
	if err := os.WriteFile(sourcePath, []byte(source), 0o644); err != nil {
		t.Fatal(err)
	}
	exePath := filepath.Join(dir, "runtime_test")
	runtimeDir := filepath.Join(repoRoot(t), "runtime")
	args := []string{sourcePath}
	args = append(args, runtimebuild.SourceFiles(runtimeDir, runtime.GOOS)...)
	args = append(args, "-I", runtimeDir, "-o", exePath, "-lm")
	if output, err := exec.Command("cc", args...).CombinedOutput(); err != nil {
		t.Fatalf("cc failed: %v\n%s\n%s", err, string(output), source)
	}
	return exePath
}

func repoRoot(t *testing.T) string {
	t.Helper()
	wd, err := os.Getwd()
	if err != nil {
		t.Fatal(err)
	}
	return filepath.Clean(filepath.Join(wd, "..", ".."))
}
