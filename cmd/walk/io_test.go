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

func TestRuntimeOwnedTextInputAndParseResults(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"imp: io",
		"imp: parse",
		"",
		"var: first = io.read_line()",
		"var: second = io.read_line()",
		"var: rest = io.read_all()",
		"var: age = parse.int(first.value)",
		"var: bad_age = parse.int(second.value)",
		"var: ratio = parse.float('2.5')",
		"var: bad_ratio = parse.float('2.5x')",
		"var: flag = parse.bool('true')",
		"var: bad_flag = parse.bool('yes')",
		"out: first.ok",
		"out: first.value",
		"out: first.error",
		"out: second.ok",
		"out: rest.ok",
		"out: rest.value",
		"out: age.ok",
		"out: age.value",
		"out: age.error",
		"out: bad_age.ok",
		"out: bad_age.error",
		"out: ratio.ok",
		"out: ratio.value",
		"out: bad_ratio.ok",
		"out: bad_ratio.error",
		"out: flag.ok",
		"out: flag.value",
		"out: bad_flag.ok",
		"out: bad_flag.error",
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

	command := exec.Command(exePath)
	command.Stdin = strings.NewReader("41\nnope\ntail\nbody")
	output, err := command.CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\noutput:\n%s\nC:\n%s", err, string(output), cCode)
	}
	want := strings.Join([]string{
		"true",
		"41",
		"",
		"true",
		"true",
		"tail",
		"body",
		"true",
		"41",
		"",
		"false",
		"invalid int",
		"true",
		"2.5",
		"false",
		"invalid float",
		"true",
		"true",
		"false",
		"invalid bool",
		"",
	}, "\n")
	if got := string(output); got != want {
		t.Fatalf("stdout mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
}

func TestReadLineReportsImmediateEOFAsData(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"imp: io",
		"",
		"var: line = io.read_line()",
		"out: line.ok",
		"out: line.value",
		"out: line.error",
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

	command := exec.Command(exePath)
	command.Stdin = strings.NewReader("")
	output, err := command.CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\noutput:\n%s\nC:\n%s", err, string(output), cCode)
	}
	if got, want := string(output), "false\n\neof\n"; got != want {
		t.Fatalf("stdout mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
}
