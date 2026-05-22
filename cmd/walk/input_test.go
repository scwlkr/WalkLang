package main

import (
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
)

func TestInExpressionReadsRequiredLinesAndPrompts(t *testing.T) {
	requireCC(t)

	sourcePath := filepath.Join(repoRoot(t), "tests", "compat", "v1", "input.walk")

	cCode, warnings, err := compileFileToCWithOptions(sourcePath, false)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}

	stdout, stderr, err := runCProgramWithInput(t, cCode, "  Walker \n\nLine\r\nFinal")
	if err != nil {
		t.Fatalf("program failed: %v\nstdout:\n%s\nstderr:\n%s\nC:\n%s", err, stdout, stderr, cCode)
	}
	if want := "Name?   Walker \n\nLine\nFinal\n"; stdout != want {
		t.Fatalf("stdout mismatch:\nwant %q\ngot  %q\nstderr:\n%s\nC:\n%s", want, stdout, stderr, cCode)
	}
	if stderr != "" {
		t.Fatalf("unexpected stderr: %q", stderr)
	}
}

func TestInExpressionErrorsOnImmediateEOF(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"var: name = in:",
		"out: name",
		"",
	}, "\n"))

	cCode, warnings, err := compileFileToCWithOptions(sourcePath, false)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}

	stdout, stderr, err := runCProgramWithInput(t, cCode, "")
	if err == nil {
		t.Fatalf("expected EOF failure, got success with stdout %q stderr %q", stdout, stderr)
	}
	if !strings.Contains(stderr, "walk runtime error: input reached EOF") {
		t.Fatalf("missing EOF runtime error:\nstdout:\n%s\nstderr:\n%s", stdout, stderr)
	}
}

func TestInPromptMustBeString(t *testing.T) {
	sourcePath := filepath.Join(repoRoot(t), "tests", "fail", "bad_input_prompt.walk")

	_, err := checkFile(sourcePath)
	if err == nil {
		t.Fatal("expected prompt type error")
	}
	if got, want := err.Error(), "tests/fail/bad_input_prompt.walk:1:13: type error: in prompt must be string, got int"; !strings.HasSuffix(got, want) {
		t.Fatalf("want suffix %q, got %q", want, got)
	}
}

func runCProgramWithInput(t *testing.T, cCode string, stdin string) (string, string, error) {
	t.Helper()
	dir := t.TempDir()
	exePath := filepath.Join(dir, "program")
	if err := buildC(cCode, filepath.Join(dir, "program.c"), exePath, nativeBuildOptions{}); err != nil {
		t.Fatal(err)
	}
	command := exec.Command(exePath)
	command.Stdin = strings.NewReader(stdin)
	var stdout strings.Builder
	var stderr strings.Builder
	command.Stdout = &stdout
	command.Stderr = &stderr
	err := command.Run()
	return stdout.String(), stderr.String(), err
}
