package main

import (
	"os"
	"os/exec"
	"path/filepath"
	"slices"
	"strings"
	"testing"
)

func TestExplicitSystemsDeferRunsInLIFOOrder(t *testing.T) {
	requireCC(t)
	source := strings.Join([]string{
		"imp: io",
		"",
		"do: io.write_line('body')",
		"defer: do io.write_line('first')",
		"defer: do io.write_line('second')",
		"",
	}, "\n")
	got := runWalkProgram(t, source)
	if want := "body\nsecond\nfirst\n"; got != want {
		t.Fatalf("defer LIFO mismatch:\nwant %q\ngot  %q", want, got)
	}
}

func TestExplicitSystemsDeferRunsBeforeEarlyReturn(t *testing.T) {
	requireCC(t)
	source := strings.Join([]string{
		"imp: io",
		"",
		"func: choose(flag bool) int",
		"    defer: do io.write_line('cleanup outer')",
		"    if: flag",
		"        defer: do io.write_line('cleanup inner')",
		"        return: 7",
		"    return: 3",
		"",
		"out: choose(true)",
		"",
	}, "\n")
	got := runWalkProgram(t, source)
	if want := "cleanup inner\ncleanup outer\n7\n"; got != want {
		t.Fatalf("defer before return mismatch:\nwant %q\ngot  %q", want, got)
	}
}

func TestExplicitSystemsLoopBodyDefersRunPerIterationWithCapturedArgs(t *testing.T) {
	requireCC(t)
	source := strings.Join([]string{
		"imp: io",
		"",
		"var: labels = ['one', 'two']",
		"for: label in labels",
		"    defer: do io.write_line(label)",
		"    do: io.write_line('body')",
		"",
	}, "\n")
	got := runWalkProgram(t, source)
	if want := "body\none\nbody\ntwo\n"; got != want {
		t.Fatalf("loop defer mismatch:\nwant %q\ngot  %q", want, got)
	}
}

func TestExplicitSystemsRecoverableResultDocsAndRuntimePolicy(t *testing.T) {
	requireCC(t)

	resultSource := strings.Join([]string{
		"imp: file",
		"imp: parse",
		"",
		"var: read = file.try_read('missing.txt')",
		"out: read.ok",
		"out: read.value",
		"out: read.error",
		"var: age = parse.int('not-an-int')",
		"out: age.ok",
		"out: age.value",
		"out: age.error",
		"",
	}, "\n")
	got := runWalkProgram(t, resultSource)
	if want := "false\n\nfile read failed\nfalse\n0\ninvalid int\n"; got != want {
		t.Fatalf("recoverable result mismatch:\nwant %q\ngot  %q", want, got)
	}

	failStopSource := strings.Join([]string{
		"imp: term",
		"",
		"do: term.color('orange')",
		"",
	}, "\n")
	stderr := runWalkProgramExpectFailure(t, failStopSource)
	if want := "walk runtime error: term color unknown\n"; stderr != want {
		t.Fatalf("fail-stop mismatch:\nwant %q\ngot  %q", want, stderr)
	}

	conformanceDocs := []struct {
		path string
		want string
	}{
		{"docs/LANGUAGE_CONCEPTS.md", "### Recoverable Result Data"},
		{"docs/DESIGN_RULES.md", "Recoverable result values are the preferred policy for ordinary documented failures."},
		{"docs/STDLIB.md", "Failure policy: recoverable result data."},
		{"docs/STDLIB.md", "Failure policy: fail-stop runtime failure."},
		{"docs/ERRORS.md", "Recoverable result data is runtime data, not a diagnostic, failed assertion, or runtime failure."},
	}
	root := repoRoot(t)
	for _, doc := range conformanceDocs {
		assertFileContains(t, filepath.Join(root, doc.path), doc.want)
	}
}

func TestExplicitSystemsBuildModeArgsAndConfig(t *testing.T) {
	debugArgs := nativeBuildArgs("main.c", "main", nativeBuildOptions{})
	for _, want := range []string{"-g", "-O0"} {
		if !slices.Contains(debugArgs, want) {
			t.Fatalf("debug build args missing %q: %#v", want, debugArgs)
		}
	}

	releaseArgs := nativeBuildArgs("main.c", "main", nativeBuildOptions{release: true})
	for _, want := range []string{"-O3", "-DNDEBUG"} {
		if !slices.Contains(releaseArgs, want) {
			t.Fatalf("release build args missing %q: %#v", want, releaseArgs)
		}
	}

	parsed, err := parseBuildArgs([]string{"--mode", "debug", "--cflag", "-DWALK_TEST", "main.walk", "-o", "main"})
	if err != nil {
		t.Fatalf("--mode debug should parse: %v", err)
	}
	if parsed.native.release {
		t.Fatal("--mode debug should not set release mode")
	}

	if _, err := parseRunArgs([]string{"--mode", "release", "--debug", "main.walk"}); err == nil || err.Error() != "build mode conflict: choose one of debug or release" {
		t.Fatalf("expected run mode conflict, got %v", err)
	}
	if _, err := parseBuildArgs([]string{"--mode", "fast", "main.walk", "-o", "main"}); err == nil || err.Error() != "unknown build mode \"fast\"" {
		t.Fatalf("expected unknown mode, got %v", err)
	}

	config, err := parseProjectConfig(strings.Join([]string{
		"name = \"hello\"",
		"",
		"[build]",
		"output = \"build/hello\"",
		"release = true",
		"mode = \"debug\"",
		"",
	}, "\n"), "walk.toml")
	if err != nil {
		t.Fatalf("project build mode should parse: %v", err)
	}
	if config.build.release {
		t.Fatal("[build].mode should win over release compatibility flag")
	}
	if len(config.warnings) != 1 || config.warnings[0] != "walk.toml:6: warning: [build].mode overrides [build].release" {
		t.Fatalf("expected mode override warning, got %#v", config.warnings)
	}
}

func TestPackageCollectionsReserveStdAndBuiltinRoots(t *testing.T) {
	dir := t.TempDir()
	err := run([]string{"package", "init", filepath.Join(dir, "std")})
	if err == nil || err.Error() != "package error: package name \"std\" is reserved for a built-in collection root" {
		t.Fatalf("expected std reservation, got %v", err)
	}

	mathDir := filepath.Join(dir, "math")
	if err := run([]string{"package", "init", mathDir}); err != nil {
		t.Fatal(err)
	}
	err = withWorkingDir(mathDir, func() error {
		return run([]string{"package", "publish", filepath.Join(dir, "registry")})
	})
	if err == nil || !strings.Contains(err.Error(), "package name \"math\" is reserved for a built-in collection root") {
		t.Fatalf("expected reserved built-in publish error, got %v", err)
	}
}

func runWalkProgram(t *testing.T, source string) string {
	t.Helper()
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, source)
	cCode, warnings, err := compileFileToCWithOptions(sourcePath, false)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}
	return runCProgram(t, cCode)
}

func runWalkProgramExpectFailure(t *testing.T, source string) string {
	t.Helper()
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, source)
	cCode, warnings, err := compileFileToCWithOptions(sourcePath, false)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}
	exePath := filepath.Join(dir, "program")
	if err := buildC(cCode, filepath.Join(dir, "program.c"), exePath, nativeBuildOptions{}); err != nil {
		t.Fatal(err)
	}
	output, err := exec.Command(exePath).CombinedOutput()
	if err == nil {
		t.Fatalf("expected runtime failure, got success with output %q", string(output))
	}
	return string(output)
}

func assertFileContains(t *testing.T, path string, want string) {
	t.Helper()
	contents, err := os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(string(contents), want) {
		t.Fatalf("%s missing %q", path, want)
	}
}
