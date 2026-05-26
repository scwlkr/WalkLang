package runtime_test

import (
	"io"
	"net/http"
	"net/http/httptest"
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

func TestRuntimeCoversDraftResultDataHelpers(t *testing.T) {
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
		`    IOReadResult line = walk_rt_io_read_line();`,
		`    check(line.ok && strcmp(line.value, "first") == 0 && strcmp(line.error, "") == 0, "io read_line result failed");`,
		`    IOReadResult rest = walk_rt_io_read_all();`,
		`    check(rest.ok && strcmp(rest.value, "tail") == 0, "io read_all result failed");`,
		``,
		`    ParseIntResult parsed_int = walk_rt_parse_int("42");`,
		`    check(parsed_int.ok && parsed_int.value == 42 && strcmp(parsed_int.error, "") == 0, "parse int result failed");`,
		`    ParseIntResult invalid_int = walk_rt_parse_int("x");`,
		`    check(!invalid_int.ok && strcmp(invalid_int.error, "invalid int") == 0, "parse int error failed");`,
		`    ParseFloatResult parsed_float = walk_rt_parse_float("2.5");`,
		`    check(parsed_float.ok && parsed_float.value > 2.49 && parsed_float.value < 2.51, "parse float result failed");`,
		`    ParseBoolResult parsed_bool = walk_rt_parse_bool("false");`,
		`    check(parsed_bool.ok && !parsed_bool.value, "parse bool result failed");`,
		``,
		`    JsonResult parsed_json = walk_rt_json_parse(" { \"name\" : \"walk\", \"ok\" : true } ");`,
		`    check(parsed_json.ok && strcmp(parsed_json.value, "{\"name\":\"walk\",\"ok\":true}") == 0, "json parse result failed");`,
		`    JsonResult invalid_json = walk_rt_json_parse("{bad");`,
		`    check(!invalid_json.ok && strcmp(invalid_json.error, "invalid json") == 0, "json parse error failed");`,
		`    check(strcmp(walk_rt_json_stringify("hi \"walk\"\n"), "\"hi \\\"walk\\\"\\n\"") == 0, "json stringify failed");`,
		`    WalkString json_path = walk_rt_path_join(argv[1], "data.json");`,
		`    walk_rt_json_write(json_path, parsed_json.value);`,
		`    JsonResult loaded_json = walk_rt_json_read(json_path);`,
		`    check(loaded_json.ok && strcmp(loaded_json.value, parsed_json.value) == 0, "json read/write failed");`,
		``,
		`    check(strcmp(walk_rt_html_escape("<tag & \"quote\">"), "&lt;tag &amp; &quot;quote&quot;&gt;") == 0, "html escape failed");`,
		`    check(strcmp(walk_rt_html_h1("Walk <Lang>"), "<h1>Walk &lt;Lang&gt;</h1>") == 0, "html h1 failed");`,
		`    check(strcmp(walk_rt_html_p("copy & text"), "<p>copy &amp; text</p>") == 0, "html p failed");`,
		`    check(strcmp(walk_rt_html_button("Go"), "<button>Go</button>") == 0, "html button failed");`,
		``,
		`    printf("ok\n");`,
		`    return 0;`,
		`}`,
	}, "\n"))

	command := exec.Command(exePath, t.TempDir())
	command.Stdin = strings.NewReader("first\ntail")
	output, err := command.CombinedOutput()
	if err != nil {
		t.Fatalf("runtime draft result helper program failed: %v\n%s", err, string(output))
	}
	if got, want := string(output), "ok\n"; got != want {
		t.Fatalf("runtime draft result helper output mismatch: want %q got %q", want, got)
	}
}

func TestRuntimeHTTPPreservesRecoverableResultData(t *testing.T) {
	if _, err := exec.LookPath("curl"); err != nil {
		t.Skip("draft http runtime needs curl on PATH")
	}
	server := httptest.NewServer(http.HandlerFunc(func(response http.ResponseWriter, request *http.Request) {
		switch request.URL.Path {
		case "/ok":
			_, _ = response.Write([]byte("ok-body"))
		case "/missing":
			response.WriteHeader(http.StatusNotFound)
			_, _ = response.Write([]byte("missing-body"))
		case "/echo":
			body, _ := io.ReadAll(request.Body)
			_, _ = response.Write([]byte(request.Method + ":" + string(body)))
		default:
			response.WriteHeader(http.StatusTeapot)
		}
	}))
	defer server.Close()

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
		`    check(argc == 4, "expected URL arguments");`,
		`    HttpResult ok = walk_rt_http_get(argv[1]);`,
		`    check(ok.ok && ok.status == 200 && strcmp(ok.body, "ok-body") == 0 && strcmp(ok.error, "") == 0, "http get result failed");`,
		`    HttpResult missing = walk_rt_http_get(argv[2]);`,
		`    check(!missing.ok && missing.status == 404 && strcmp(missing.body, "missing-body") == 0 && strcmp(missing.error, "http status") == 0, "http status result failed");`,
		`    HttpResult posted = walk_rt_http_post(argv[3], "hello");`,
		`    check(posted.ok && posted.status == 200 && strcmp(posted.body, "POST:hello") == 0, "http post result failed");`,
		`    HttpResult empty = walk_rt_http_get("");`,
		`    check(!empty.ok && empty.status == -1 && strcmp(empty.error, "http url empty") == 0, "http empty URL error failed");`,
		`    printf("ok\n");`,
		`    return 0;`,
		`}`,
	}, "\n"))

	output, err := exec.Command(exePath, server.URL+"/ok", server.URL+"/missing", server.URL+"/echo").CombinedOutput()
	if err != nil {
		t.Fatalf("runtime http program failed: %v\n%s", err, string(output))
	}
	if got, want := string(output), "ok\n"; got != want {
		t.Fatalf("runtime http output mismatch: want %q got %q", want, got)
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
