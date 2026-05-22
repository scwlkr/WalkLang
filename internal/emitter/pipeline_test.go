package emitter_test

import (
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"

	"walklang/internal/checker"
	"walklang/internal/emitter"
	walkfmt "walklang/internal/format"
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

func TestV0RoadmapRepresentativeProgramBuildsAndRuns(t *testing.T) {
	if _, err := exec.LookPath("cc"); err != nil {
		t.Skip("cc is not available")
	}

	cCode := compileToC(t, strings.Join([]string{
		"imp: math",
		"",
		"func: add(a int, b int) int",
		"    return: + a b",
		"",
		"func: distance(x1 float, y1 float, x2 float, y2 float) float",
		"    return:",
		"        math.sqrt(",
		"            +:",
		"                ^ (- x2 x1) 2",
		"                ^ (- y2 y1) 2",
		"        )",
		"",
		"var: nums = [1, 2, 3]",
		"",
		"for: n in nums",
		"    out: add(n, 10)",
		"",
		"var: d = distance(0, 0, 3, 4)",
		"",
		"if: == d 5",
		"    out: 'distance is 5'",
		"else:",
		"    out: 'distance is not 5'",
	}, "\n"))

	dir := t.TempDir()
	cPath := filepath.Join(dir, "v0.c")
	exePath := filepath.Join(dir, "v0")
	if err := os.WriteFile(cPath, []byte(cCode), 0o644); err != nil {
		t.Fatal(err)
	}
	if output, err := exec.Command("cc", cPath, "-o", exePath, "-lm").CombinedOutput(); err != nil {
		t.Fatalf("cc failed: %v\n%s\n%s", err, string(output), cCode)
	}
	output, err := exec.Command(exePath).CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\n%s", err, string(output))
	}
	if got, want := string(output), "11\n12\n13\ndistance is 5\n"; got != want {
		t.Fatalf("output mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
}

func TestV0ControlFlowNullArraysAndFunctionValues(t *testing.T) {
	if _, err := exec.LookPath("cc"); err != nil {
		t.Skip("cc is not available")
	}

	cCode := compileToC(t, strings.Join([]string{
		"func: inc(x int) int",
		"    return: + x 1",
		"",
		"func: apply(f func(int) int, x int) int",
		"    return: f(x)",
		"",
		"var: nums = [1, 2, 3]",
		"nums[1] = 9",
		"for: n in nums",
		"    out: n",
		"",
		"var: count = 0",
		"while: < count 3",
		"    count = + count 1",
		"    if: == count 2",
		"        continue:",
		"    out: count",
		"",
		"repeat: 2",
		"    out: 'r'",
		"",
		"var: name string? = null",
		"if: == name null",
		"    out: 'null'",
		"name = 'ok'",
		"if: != name null",
		"    out: name",
		"",
		"out: apply(inc, 4)",
	}, "\n"))

	dir := t.TempDir()
	cPath := filepath.Join(dir, "features.c")
	exePath := filepath.Join(dir, "features")
	if err := os.WriteFile(cPath, []byte(cCode), 0o644); err != nil {
		t.Fatal(err)
	}
	if output, err := exec.Command("cc", cPath, "-o", exePath, "-lm").CombinedOutput(); err != nil {
		t.Fatalf("cc failed: %v\n%s\n%s", err, string(output), cCode)
	}
	output, err := exec.Command(exePath).CombinedOutput()
	if err != nil {
		t.Fatalf("program failed: %v\n%s", err, string(output))
	}
	want := "1\n9\n3\n1\n3\nr\nr\nnull\nok\n5\n"
	if got := string(output); got != want {
		t.Fatalf("output mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
}

func TestV01TestRunnerProgramBuildsAndRuns(t *testing.T) {
	if _, err := exec.LookPath("cc"); err != nil {
		t.Skip("cc is not available")
	}

	program, err := parser.ParseSource(strings.Join([]string{
		"imp: math",
		"imp: string",
		"imp: array",
		"imp: time",
		"",
		"func: add(a int, b int) int",
		"    return: + a b",
		"",
		"test: 'add works'",
		"    assert: == add(2, 3) 5",
		"",
		"test: 'stdlib polish works'",
		"    var: nums = [1, 2, 3]",
		"    assert: == array.len(nums) 3",
		"    assert: == string.len('walk') 4",
		"    assert: == math.pow(2, 3) 8",
		"    assert: > time.now() 0",
	}, "\n"), "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	if err := checker.Check(program); err != nil {
		t.Fatal(err)
	}
	cCode, err := emitter.EmitTestC(program)
	if err != nil {
		t.Fatal(err)
	}

	dir := t.TempDir()
	cPath := filepath.Join(dir, "tests.c")
	exePath := filepath.Join(dir, "tests")
	if err := os.WriteFile(cPath, []byte(cCode), 0o644); err != nil {
		t.Fatal(err)
	}
	if output, err := exec.Command("cc", cPath, "-o", exePath, "-lm").CombinedOutput(); err != nil {
		t.Fatalf("cc failed: %v\n%s\n%s", err, string(output), cCode)
	}
	output, err := exec.Command(exePath).CombinedOutput()
	if err != nil {
		t.Fatalf("test program failed: %v\n%s\n%s", err, string(output), cCode)
	}
	want := "test: add works\ntest: stdlib polish works\nok 2 tests\n"
	if got := string(output); got != want {
		t.Fatalf("output mismatch:\nwant %q\ngot  %q\nC:\n%s", want, got, cCode)
	}
}

func TestV01DiagnosticsIncludeLineAndColumn(t *testing.T) {
	program, err := parser.ParseSource("var: x = 1\nx = 'one'\n", "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	err = checker.Check(program)
	if err == nil {
		t.Fatal("expected type error")
	}
	if got, want := err.Error(), "main.walk:2:1: type error: x is int, got string"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}

func TestFormatterNormalizesInitialSyntax(t *testing.T) {
	formatted, err := walkfmt.Format("var:x=+ 1 2\nout:'hello'\n", "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	if got, want := formatted, "var: x = + 1 2\nout: 'hello'\n"; got != want {
		t.Fatalf("want %q, got %q", want, got)
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
	if got, want := err.Error(), "main.walk:2:1: type error: x is int, got string"; got != want {
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
	if got, want := err.Error(), "main.walk:2:1: type error: x is const and cannot be reassigned"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}

func TestReservedWordsCannotBeNames(t *testing.T) {
	_, err := parser.ParseSource("var: return = 1\n", "main.walk")
	if err == nil {
		t.Fatal("expected reserved word error")
	}
	if got, want := err.Error(), "main.walk:1:6: syntax error: reserved word \"return\" cannot be used as variable name"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}
