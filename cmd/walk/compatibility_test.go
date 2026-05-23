package main

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestV19CompatibilitySuitePassPrograms(t *testing.T) {
	requireCC(t)
	root := repoRoot(t)
	cases := []struct {
		name      string
		source    string
		testsOnly bool
		want      string
	}{
		{
			name:   "stable v1 program",
			source: filepath.Join(root, "tests", "compat", "v1", "stable.walk"),
			want: strings.Join([]string{
				"27",
				"5",
				"3",
				"8",
				"4",
				"a",
				"l",
				"true",
				"walklang",
				"3",
				"1",
				"true",
				"7",
				"stable",
				"true",
				"length 4",
				"{stable}",
				"1",
				"9",
				"3",
				"1",
				"3",
				"r",
				"r",
				"missing",
				"Walker",
				"5",
				"",
			}, "\n"),
		},
		{
			name:      "stable v1 test program",
			source:    filepath.Join(root, "tests", "compat", "v1", "tests.walk"),
			testsOnly: true,
			want:      "test: stdlib assert stays compatible\ntest: module export stays compatible\nok 2 tests\n",
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			cCode, warnings, err := compileFileToCWithOptions(tc.source, tc.testsOnly)
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
}

func TestV19CompatibilitySuiteStableFailures(t *testing.T) {
	root := repoRoot(t)
	cases := []struct {
		file string
		want string
	}{
		{"type_mismatch.walk", "tests/fail/type_mismatch.walk:2:1: type error: x is int, got string"},
		{"unknown_name.walk", "tests/fail/unknown_name.walk:1:6: name error: missing_value is not defined"},
		{"private_module_func.walk", "tests/fail/private_module_func.walk:2:6: name error: module private_math does not export hidden"},
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

func TestV19ReleaseDocsArePresent(t *testing.T) {
	root := repoRoot(t)
	cases := []struct {
		file string
		want string
	}{
		{"docs/COMPATIBILITY.md", "Stable v1 code should continue to compile through the v1.x line"},
		{"docs/INSTALL.md", "Official Install Instructions"},
		{"docs/RELEASE_NOTES.md", "v5.7.0"},
		{"docs/MIGRATING.md", "v1.8 to v1.9"},
		{"docs/DEPRECATION.md", "Current v1.9 Deprecated Surface"},
	}

	for _, tc := range cases {
		t.Run(tc.file, func(t *testing.T) {
			contents, err := os.ReadFile(filepath.Join(root, tc.file))
			if err != nil {
				t.Fatal(err)
			}
			if !strings.Contains(string(contents), tc.want) {
				t.Fatalf("%s missing %q", tc.file, tc.want)
			}
		})
	}
}
