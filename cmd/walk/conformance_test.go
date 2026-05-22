package main

import (
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
)

func TestV11PassFixturesBuildAndRun(t *testing.T) {
	requireCC(t)
	root := repoRoot(t)
	cases := []struct {
		file string
		want string
	}{
		{"hello.walk", "hello\n3\ntrue\n"},
		{"functions.walk", "5\n120\n"},
		{"arrays.walk", "1\n9\n3\nb\n"},
		{"control_flow.walk", "1\n3\nr\nr\n2\n6\n"},
		{"nullable.walk", "missing\nWalker\n"},
		{"function_values.walk", "5\n"},
		{"stdlib.walk", "3\n8\n4\n3\ntrue\n"},
		{"modules.walk", "25\n10\n"},
	}

	for _, tc := range cases {
		t.Run(tc.file, func(t *testing.T) {
			sourcePath := filepath.Join(root, "tests", "pass", tc.file)
			cCode, warnings, err := compileFileToCWithOptions(sourcePath, false)
			if err != nil {
				t.Fatal(err)
			}
			if len(warnings) != 0 {
				t.Fatalf("unexpected warnings: %#v", warnings)
			}
			got := runCProgram(t, cCode)
			if got != tc.want {
				t.Fatalf("want %q, got %q", tc.want, got)
			}
		})
	}
}

func TestV11TestFixtureRuns(t *testing.T) {
	requireCC(t)
	root := repoRoot(t)
	sourcePath := filepath.Join(root, "tests", "pass", "walk_tests.walk")
	cCode, warnings, err := compileFileToCWithOptions(sourcePath, true)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}
	got := runCProgram(t, cCode)
	want := "test: arithmetic works\ntest: stdlib works\nok 2 tests\n"
	if got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}

func TestV11FailFixturesHaveExpectedDiagnostics(t *testing.T) {
	root := repoRoot(t)
	cases := []struct {
		file string
		want string
	}{
		{"type_mismatch.walk", "tests/fail/type_mismatch.walk:2:1: type error: x is int, got string"},
		{"unknown_name.walk", "tests/fail/unknown_name.walk:1:6: name error: missing_value is not defined"},
		{"bad_indent.walk", "tests/fail/bad_indent.walk:2:5: syntax error: unexpected indented block"},
		{"private_module_func.walk", "tests/fail/private_module_func.walk:2:6: name error: module private_math does not export hidden"},
		{"const_reassign.walk", "tests/fail/const_reassign.walk:2:1: type error: limit is const and cannot be reassigned"},
		{"missing_return.walk", "tests/fail/missing_return.walk:1:1: type error: function bad may not return on all paths"},
		{"bad_array.walk", "tests/fail/bad_array.walk:1:15: type error: arrays must be homogeneous, got int and string"},
		{"non_bool_if.walk", "tests/fail/non_bool_if.walk:1:5: type error: if condition must be bool, got int"},
		{"bad_assert.walk", "tests/fail/bad_assert.walk:2:13: type error: assert needs bool, got int"},
		{"unknown_library.walk", "tests/fail/unknown_library.walk:2:6: name error: unknown library function math.nope"},
		{"top_break.walk", "tests/fail/top_break.walk:1:1: syntax error: break outside loop"},
	}

	for _, tc := range cases {
		t.Run(tc.file, func(t *testing.T) {
			_, err := checkFile(filepath.Join(root, "tests", "fail", tc.file))
			if err == nil {
				t.Fatal("expected fixture to fail")
			}
			if got := err.Error(); !strings.HasSuffix(got, tc.want) {
				t.Fatalf("want suffix %q, got %q", tc.want, got)
			}
		})
	}
}

func TestV11GeneratedCSnapshots(t *testing.T) {
	root := repoRoot(t)
	for _, name := range []string{"hello", "functions"} {
		t.Run(name, func(t *testing.T) {
			sourcePath := filepath.Join(root, "tests", "pass", name+".walk")
			got, warnings, err := compileFileToCWithOptions(sourcePath, false)
			if err != nil {
				t.Fatal(err)
			}
			if len(warnings) != 0 {
				t.Fatalf("unexpected warnings: %#v", warnings)
			}
			wantBytes, err := os.ReadFile(filepath.Join(root, "tests", "snapshots", name+".c"))
			if err != nil {
				t.Fatal(err)
			}
			if got != string(wantBytes) {
				t.Fatalf("generated C snapshot changed for %s.walk", name)
			}
		})
	}
}

func TestV12ExamplesAreTestableFixtures(t *testing.T) {
	requireCC(t)
	root := repoRoot(t)
	programs := []struct {
		file string
		want string
	}{
		{"hello.walk", "3\nhello from WalkLang\ntrue\n"},
		{"v0.walk", "11\n12\n13\ndistance is 5\n"},
		{"v1.walk", "27\n8\n4\n3\ntrue\n"},
	}
	for _, tc := range programs {
		t.Run(tc.file, func(t *testing.T) {
			cCode, warnings, err := compileFileToCWithOptions(filepath.Join(root, "examples", tc.file), false)
			if err != nil {
				t.Fatal(err)
			}
			if len(warnings) != 0 {
				t.Fatalf("unexpected warnings: %#v", warnings)
			}
			if got := runCProgram(t, cCode); got != tc.want {
				t.Fatalf("want %q, got %q", tc.want, got)
			}
		})
	}

	cCode, warnings, err := compileFileToCWithOptions(filepath.Join(root, "examples", "v0_1_tests.walk"), true)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}
	want := "test: add works\ntest: stdlib polish works\nok 2 tests\n"
	if got := runCProgram(t, cCode); got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}

func repoRoot(t *testing.T) string {
	t.Helper()
	wd, err := os.Getwd()
	if err != nil {
		t.Fatal(err)
	}
	return filepath.Clean(filepath.Join(wd, "..", ".."))
}

func requireCC(t *testing.T) {
	t.Helper()
	if _, err := exec.LookPath("cc"); err != nil {
		t.Skip("cc is not available")
	}
}

func runCProgram(t *testing.T, cCode string) string {
	t.Helper()
	dir := t.TempDir()
	exePath := filepath.Join(dir, "program")
	if err := buildC(cCode, filepath.Join(dir, "program.c"), exePath, nativeBuildOptions{}); err != nil {
		t.Fatal(err)
	}
	output, err := exec.Command(exePath).CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\n%s", err, string(output))
	}
	return string(output)
}
