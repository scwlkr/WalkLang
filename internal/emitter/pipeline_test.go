package emitter_test

import (
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"

	"walklang/internal/checker"
	"walklang/internal/emitter"
	"walklang/internal/parser"
)

func compileToC(t *testing.T, source string) string {
	t.Helper()
	program, err := parser.ParseSource(source, "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	if err := checker.Check(program); err != nil {
		t.Fatal(err)
	}
	cCode, err := emitter.EmitC(program)
	if err != nil {
		t.Fatal(err)
	}
	return cCode
}

func TestEmitCForVariablesAndOutput(t *testing.T) {
	cCode := compileToC(t, strings.Join([]string{
		"var: x = + 1 2",
		"const: ok = true",
		"out: x",
		"out: ok",
	}, "\n"))

	for _, want := range []string{
		"long long x = (1 + 2);",
		"const bool ok = true;",
		"printf(\"%lld\\n\", (long long)(x));",
		"printf(\"%s\\n\", (ok) ? \"true\" : \"false\");",
	} {
		if !strings.Contains(cCode, want) {
			t.Fatalf("generated C missing %q:\n%s", want, cCode)
		}
	}
}

func TestGeneratedCBuildsAndRuns(t *testing.T) {
	if _, err := exec.LookPath("cc"); err != nil {
		t.Skip("cc is not available")
	}

	cCode := compileToC(t, strings.Join([]string{
		"var: x = + 1 2",
		"out: x",
		"out: 'ok'",
		"out: / 5 2",
	}, "\n"))

	dir := t.TempDir()
	cPath := filepath.Join(dir, "main.c")
	exePath := filepath.Join(dir, "main")
	if err := os.WriteFile(cPath, []byte(cCode), 0o644); err != nil {
		t.Fatal(err)
	}
	if output, err := exec.Command("cc", cPath, "-o", exePath, "-lm").CombinedOutput(); err != nil {
		t.Fatalf("cc failed: %v\n%s", err, string(output))
	}
	output, err := exec.Command(exePath).CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\n%s", err, string(output))
	}
	if got, want := string(output), "3\nok\n2.5\n"; got != want {
		t.Fatalf("output mismatch:\nwant %q\ngot  %q", want, got)
	}
}

func TestStringEqualityUsesValueComparison(t *testing.T) {
	cCode := compileToC(t, strings.Join([]string{
		"var: name = 'WalkLang'",
		"out: == name 'WalkLang'",
		"out: != name 'Python'",
	}, "\n"))

	for _, want := range []string{
		"#include <string.h>",
		`strcmp(name, "WalkLang") == 0`,
		`strcmp(name, "Python") != 0`,
	} {
		if !strings.Contains(cCode, want) {
			t.Fatalf("generated C missing %q:\n%s", want, cCode)
		}
	}
}

func TestTypeErrors(t *testing.T) {
	program, err := parser.ParseSource("var: x = 1\nx = 'one'\n", "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	err = checker.Check(program)
	if err == nil {
		t.Fatal("expected type error")
	}
	if got, want := err.Error(), "main.walk:2: type error: x is int, got string"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}

func TestConstAssignmentIsRejected(t *testing.T) {
	program, err := parser.ParseSource("const: x = 1\nx = 2\n", "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	err = checker.Check(program)
	if err == nil {
		t.Fatal("expected const reassignment error")
	}
	if got, want := err.Error(), "main.walk:2: type error: x is const and cannot be reassigned"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}

func TestReservedWordsCannotBeNames(t *testing.T) {
	_, err := parser.ParseSource("var: return = 1\n", "main.walk")
	if err == nil {
		t.Fatal("expected reserved word error")
	}
	if got, want := err.Error(), "main.walk:1: syntax error: reserved word \"return\" cannot be used as variable name"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}
