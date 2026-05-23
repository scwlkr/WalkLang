package main

import (
	"errors"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
)

func TestDoEffectConsoleAndProcessFoundation(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"imp: array",
		"imp: io",
		"imp: process",
		"imp: string",
		"",
		"do: io.write('prefix')",
		"do: io.write_line('-line')",
		"do: io.error_line('err-line')",
		"out: process.arg_count()",
		"var: args = process.args()",
		"out: array.len(args)",
		"out: args[0]",
		"out: args[1]",
		"out: process.env('WALK_IO_TEST')",
		"out: > string.len(process.cwd()) 0",
		"",
	}, "\n"))

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

	command := exec.Command(exePath, "left", "right")
	command.Dir = dir
	command.Env = append(os.Environ(), "WALK_IO_TEST=env-ok")
	var stdout strings.Builder
	var stderr strings.Builder
	command.Stdout = &stdout
	command.Stderr = &stderr
	if err := command.Run(); err != nil {
		t.Fatalf("program failed: %v\nstdout:\n%s\nstderr:\n%s\nC:\n%s", err, stdout.String(), stderr.String(), cCode)
	}
	if got, want := stdout.String(), "prefix-line\n2\n2\nleft\nright\nenv-ok\ntrue\n"; got != want {
		t.Fatalf("stdout mismatch:\nwant %q\ngot  %q\nstderr:\n%s\nC:\n%s", want, got, stderr.String(), cCode)
	}
	if got, want := stderr.String(), "err-line\n"; got != want {
		t.Fatalf("stderr mismatch:\nwant %q\ngot  %q\nstdout:\n%s\nC:\n%s", want, got, stdout.String(), cCode)
	}
}

func TestProcessExitEffectExitsWithCode(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"imp: process",
		"",
		"do: process.exit(7)",
		"out: 'after'",
		"",
	}, "\n"))

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
	var exitErr *exec.ExitError
	if !errors.As(err, &exitErr) {
		t.Fatalf("expected exit error, got %v with output %q", err, string(output))
	}
	if code := exitErr.ExitCode(); code != 7 {
		t.Fatalf("want exit code 7, got %d with output %q", code, string(output))
	}
	if len(output) != 0 {
		t.Fatalf("process.exit should stop before later output, got %q", string(output))
	}
}
