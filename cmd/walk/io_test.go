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

func TestDraftFileTextIOReadsWritesAndChecksExistence(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"imp: file",
		"",
		"out: file.exists('note.txt')",
		"do: file.write('note.txt', 'hello from WalkLang')",
		"out: file.exists('note.txt')",
		"out: file.read('note.txt')",
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
	command.Dir = dir
	output, err := command.CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\noutput:\n%s\nC:\n%s", err, string(output), cCode)
	}
	if got, want := string(output), "false\ntrue\nhello from WalkLang\n"; got != want {
		t.Fatalf("stdout mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
	contents, err := os.ReadFile(filepath.Join(dir, "note.txt"))
	if err != nil {
		t.Fatal(err)
	}
	if got, want := string(contents), "hello from WalkLang"; got != want {
		t.Fatalf("file contents mismatch: want %q, got %q", want, got)
	}
}

func TestDraftLocalFilesystemPhase2CompletesFileDirPathAndCwd(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"imp: array",
		"imp: dir",
		"imp: file",
		"imp: path",
		"imp: process",
		"",
		"var: data_dir = 'data'",
		"var: note_path = path.join(data_dir, 'note.txt')",
		"out: path.base(note_path)",
		"out: path.ext(note_path)",
		"out: file.exists(data_dir)",
		"do: dir.make(data_dir)",
		"out: file.exists(data_dir)",
		"do: file.write(note_path, 'alpha')",
		"do: file.append(note_path, '\\nbeta')",
		"out: file.read(note_path)",
		"var: files = dir.list(data_dir)",
		"out: array.len(files)",
		"out: array.contains(files, 'note.txt')",
		"do: dir.make(path.join(data_dir, 'empty'))",
		"out: file.exists(path.join(data_dir, 'empty'))",
		"do: dir.delete(path.join(data_dir, 'empty'))",
		"out: file.exists(path.join(data_dir, 'empty'))",
		"var: original = process.cwd()",
		"do: process.chdir(data_dir)",
		"out: path.base(process.cwd())",
		"out: file.read('note.txt')",
		"do: process.chdir(original)",
		"out: path.base(process.cwd())",
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
	command.Dir = dir
	output, err := command.CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\noutput:\n%s\nC:\n%s", err, string(output), cCode)
	}
	want := strings.Join([]string{
		"note.txt",
		".txt",
		"false",
		"true",
		"alpha",
		"beta",
		"1",
		"true",
		"true",
		"false",
		"data",
		"alpha",
		"beta",
		filepath.Base(dir),
		"",
	}, "\n")
	if got := string(output); got != want {
		t.Fatalf("stdout mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
}

func TestDraftRecoverableFileResults(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"imp: file",
		"",
		"var: missing = file.try_read('missing.txt')",
		"out: missing.ok",
		"out: missing.value",
		"out: missing.error",
		"var: wrote = file.try_write('note.txt', 'hello')",
		"out: wrote.ok",
		"out: wrote.value",
		"out: wrote.error",
		"var: appended = file.try_append('note.txt', ' world')",
		"out: appended.ok",
		"out: appended.value",
		"out: appended.error",
		"var: read = file.try_read('note.txt')",
		"out: read.ok",
		"out: read.value",
		"out: read.error",
		"var: bad_write = file.try_write('', 'nope')",
		"out: bad_write.ok",
		"out: bad_write.value",
		"out: bad_write.error",
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
	command.Dir = dir
	output, err := command.CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\noutput:\n%s\nC:\n%s", err, string(output), cCode)
	}
	want := strings.Join([]string{
		"false",
		"",
		"file read failed",
		"true",
		"true",
		"",
		"true",
		"true",
		"",
		"true",
		"hello world",
		"",
		"false",
		"false",
		"file path empty",
		"",
	}, "\n")
	if got := string(output); got != want {
		t.Fatalf("stdout mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
}

func TestDraftFileReadMissingFileFailsClearly(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"imp: file",
		"",
		"out: file.read('missing.txt')",
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
	command.Dir = dir
	output, err := command.CombinedOutput()
	var exitErr *exec.ExitError
	if !errors.As(err, &exitErr) {
		t.Fatalf("expected exit error, got %v with output %q", err, string(output))
	}
	if code := exitErr.ExitCode(); code == 0 {
		t.Fatalf("expected non-zero exit code, got %d with output %q", code, string(output))
	}
	if got, want := string(output), "walk runtime error: file read failed\n"; got != want {
		t.Fatalf("runtime error mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
}

func TestDraftFileReadInvalidUTF8FailsClearly(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "bad.txt"), []byte{0xff}, 0o644); err != nil {
		t.Fatal(err)
	}
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"imp: file",
		"",
		"out: file.read('bad.txt')",
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
	command.Dir = dir
	output, err := command.CombinedOutput()
	var exitErr *exec.ExitError
	if !errors.As(err, &exitErr) {
		t.Fatalf("expected exit error, got %v with output %q", err, string(output))
	}
	if code := exitErr.ExitCode(); code == 0 {
		t.Fatalf("expected non-zero exit code, got %d with output %q", code, string(output))
	}
	if got, want := string(output), "walk runtime error: file invalid utf-8\n"; got != want {
		t.Fatalf("runtime error mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
}
