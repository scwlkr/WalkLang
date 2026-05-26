package main

import (
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"slices"
	"strings"
	"testing"

	"walklang/internal/diagnostic"
)

func TestUserModuleBuildsAndRuns(t *testing.T) {
	if _, err := exec.LookPath("cc"); err != nil {
		t.Skip("cc is not available")
	}

	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "math_extra.walk"), strings.Join([]string{
		"func: cube(x int) int",
		"    return: * x x x",
		"",
		"func: hidden(x int) int",
		"    return: + x 1",
		"",
		"exp: cube",
		"",
	}, "\n"))
	mainPath := filepath.Join(dir, "main.walk")
	writeFile(t, mainPath, strings.Join([]string{
		"imp: math_extra",
		"out: math_extra.cube(3)",
		"",
	}, "\n"))

	cCode, warnings, err := compileFileToCWithOptions(mainPath, false)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}

	cPath := filepath.Join(dir, "main.c")
	exePath := filepath.Join(dir, "main")
	if err := buildC(cCode, cPath, exePath, nativeBuildOptions{}); err != nil {
		t.Fatalf("cc failed: %v\n%s", err, cCode)
	}
	output, err := exec.Command(exePath).CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\n%s", err, string(output))
	}
	if got, want := string(output), "27\n"; got != want {
		t.Fatalf("want %q, got %q\nC:\n%s", want, got, cCode)
	}
	if !strings.Contains(cCode, "math_extra__cube") {
		t.Fatalf("module function was not namespaced in generated C:\n%s", cCode)
	}
}

func TestUserModuleExportsAreEnforced(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "math_extra.walk"), strings.Join([]string{
		"func: cube(x int) int",
		"    return: * x x x",
		"",
		"exp: cube",
		"",
	}, "\n"))
	mainPath := filepath.Join(dir, "main.walk")
	writeFile(t, mainPath, "imp: math_extra\nout: math_extra.hidden(3)\n")

	_, _, err := compileFileToCWithOptions(mainPath, false)
	if err == nil {
		t.Fatal("expected missing export error")
	}
	if got, want := err.Error(), "main.walk:2:6: name error: module math_extra does not export hidden"; !strings.HasSuffix(got, want) {
		t.Fatalf("want suffix %q, got %q", want, got)
	}
}

func TestUserModuleRejectsTopLevelRuntimeStatements(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "bad_module.walk"), strings.Join([]string{
		"var: x = 1",
		"",
	}, "\n"))
	mainPath := filepath.Join(dir, "main.walk")
	writeFile(t, mainPath, "imp: bad_module\n")

	_, _, err := compileFileToCWithOptions(mainPath, false)
	if err == nil {
		t.Fatal("expected module surface error")
	}
	if got, want := err.Error(), "bad_module.walk:1:1: module error: modules may contain only imp, struct, func, and exp at top level"; !strings.HasSuffix(got, want) {
		t.Fatalf("want suffix %q, got %q", want, got)
	}
}

func TestReleaseBuildArgs(t *testing.T) {
	args := nativeBuildArgs("main.c", "main", nativeBuildOptions{
		release: true,
		cFlags:  []string{"-DWALK_TEST"},
	}, filepath.Join(repoRoot(t), "runtime"))

	for _, want := range []string{"main.c", "walk_runtime.c", "walk_platform_posix.c", "-I", "-o", "main", "-O3", "-DNDEBUG", "-DWALK_TEST", "-lm"} {
		if !slices.Contains(args, want) {
			if !strings.Contains(strings.Join(args, " "), want) {
				t.Fatalf("build args missing %q: %#v", want, args)
			}
		}
	}
}

func TestCheckWarningsCanBeErrors(t *testing.T) {
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"var: x = 1",
		"if: true",
		"    var: x = 2",
		"    out: x",
		"",
	}, "\n"))

	if err := checkCommand([]string{"--warnings=off", sourcePath}); err != nil {
		t.Fatalf("warnings=off should pass: %v", err)
	}
	err := checkCommand([]string{"--warnings=error", sourcePath})
	if err == nil {
		t.Fatal("expected warnings=error to fail")
	}
	if got, want := err.Error(), "warnings-as-errors: 1 warning(s)"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}

func TestRunCommandRunsSingleFileAndDirectFileAlias(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"out: 'quick run'",
		"out: + 20 22",
		"",
	}, "\n"))

	for name, args := range map[string][]string{
		"run command":       {"run", sourcePath},
		"direct file alias": {sourcePath},
	} {
		t.Run(name, func(t *testing.T) {
			got := captureStdout(t, func() error {
				return run(args)
			})
			if want := "quick run\n42\n"; got != want {
				t.Fatalf("want %q, got %q", want, got)
			}
		})
	}
}

func TestDiagnosticFormattingIncludesSnippetCaretAndSuggestion(t *testing.T) {
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, "var: age int = 'old'\n")

	_, err := checkFile(sourcePath)
	if err == nil {
		t.Fatal("expected type error")
	}

	want := strings.Join([]string{
		sourcePath + ":1:16: type error: age is int, got string",
		"",
		"var: age int = 'old'",
		"               ^ string cannot initialize int",
	}, "\n")
	if got := diagnostic.FormatError(err); got != want {
		t.Fatalf("want:\n%s\ngot:\n%s", want, got)
	}
}

func TestWarningFormattingIncludesSnippetCaretAndSuggestion(t *testing.T) {
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"var: x = 1",
		"if: true",
		"    var: x = 2",
		"    out: x",
		"",
	}, "\n"))

	warnings, err := checkFile(sourcePath)
	if err != nil {
		t.Fatal(err)
	}
	if got, want := len(warnings), 1; got != want {
		t.Fatalf("want %d warning, got %d: %#v", want, got, warnings)
	}

	want := strings.Join([]string{
		sourcePath + ":3:5: warning: x shadows outer name",
		"",
		"    var: x = 2",
		"    ^ rename this binding or assign to the existing name",
	}, "\n")
	if got := diagnostic.FormatWarning(warnings[0].Location, warnings[0].Message); got != want {
		t.Fatalf("want:\n%s\ngot:\n%s", want, got)
	}
}

func TestUnreachableStatementWarns(t *testing.T) {
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"func: done() int",
		"    return: 1",
		"    out: 2",
		"",
		"out: done()",
		"",
	}, "\n"))

	warnings, err := checkFile(sourcePath)
	if err != nil {
		t.Fatal(err)
	}
	if got, want := len(warnings), 1; got != want {
		t.Fatalf("want %d warning, got %d: %#v", want, got, warnings)
	}
	if got, want := warnings[0].String(), sourcePath+":3:5: warning: unreachable statement"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}

func writeFile(t *testing.T, path string, contents string) {
	t.Helper()
	if err := os.WriteFile(path, []byte(contents), 0o644); err != nil {
		t.Fatal(err)
	}
}

func captureStdout(t *testing.T, fn func() error) string {
	t.Helper()

	reader, writer, err := os.Pipe()
	if err != nil {
		t.Fatal(err)
	}
	oldStdout := os.Stdout
	os.Stdout = writer
	runErr := fn()
	os.Stdout = oldStdout

	if err := writer.Close(); err != nil {
		t.Fatal(err)
	}
	output, err := io.ReadAll(reader)
	if err != nil {
		t.Fatal(err)
	}
	if err := reader.Close(); err != nil {
		t.Fatal(err)
	}
	if runErr != nil {
		t.Fatalf("run failed: %v\nstdout:\n%s", runErr, string(output))
	}
	return string(output)
}
