package main

import (
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
)

func TestV13PassFixturesBuildAndRun(t *testing.T) {
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
		{"stdlib.walk", "3\n8\n4\n3\ntrue\n7\ntrue\n"},
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

func TestV13TestFixtureRuns(t *testing.T) {
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

func TestV13FailFixturesHaveExpectedDiagnostics(t *testing.T) {
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
		{"bad_testing_assert.walk", "tests/fail/bad_testing_assert.walk:4:28: type error: testing.assert needs bool arg, got int"},
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

func TestV13GeneratedCSnapshots(t *testing.T) {
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

func TestV13ExamplesAreTestableFixtures(t *testing.T) {
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

func TestV20StructFixturesBuildAndRun(t *testing.T) {
	requireCC(t)
	root := repoRoot(t)
	cases := []struct {
		file string
		want string
	}{
		{
			file: "structs.walk",
			want: strings.Join([]string{
				"Walker",
				"25",
				"26",
				"Walker",
				"26",
				"Ada",
				"38",
				"Walker",
				"27",
				"",
			}, "\n"),
		},
		{
			file: "struct_modules.walk",
			want: "Walker\n25\n",
		},
	}

	for _, tc := range cases {
		t.Run(tc.file, func(t *testing.T) {
			cCode, warnings, err := compileFileToCWithOptions(filepath.Join(root, "tests", "pass", tc.file), false)
			if err != nil {
				t.Fatal(err)
			}
			if len(warnings) != 0 {
				t.Fatalf("unexpected warnings: %#v", warnings)
			}
			if got := runCProgram(t, cCode); got != tc.want {
				t.Fatalf("want %q, got %q\nC:\n%s", tc.want, got, cCode)
			}
		})
	}
}

func TestV20StructFailFixturesHaveExpectedDiagnostics(t *testing.T) {
	root := repoRoot(t)
	cases := []struct {
		file string
		want string
	}{
		{"missing_struct_field.walk", "tests/fail/missing_struct_field.walk:5:13: type error: User expects 2 field values, got 1"},
		{"unknown_struct_field.walk", "tests/fail/unknown_struct_field.walk:6:10: type error: User has no field height"},
		{"bad_struct_field_type.walk", "tests/fail/bad_struct_field_type.walk:5:28: type error: field age is int, got string"},
		{"const_struct_field.walk", "tests/fail/const_struct_field.walk:6:5: type error: user is const and cannot be reassigned"},
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

func TestV21MethodFixturesBuildAndRun(t *testing.T) {
	requireCC(t)
	root := repoRoot(t)
	sourcePath := filepath.Join(root, "tests", "pass", "methods.walk")
	cCode, warnings, err := compileFileToCWithOptions(sourcePath, false)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}
	got := runCProgram(t, cCode)
	want := strings.Join([]string{
		"true",
		"Ada",
		"Walker",
		"true",
		"false",
		"adult",
		"",
	}, "\n")
	if got != want {
		t.Fatalf("want %q, got %q\nC:\n%s", want, got, cCode)
	}
}

func TestV21MethodFailFixturesHaveExpectedDiagnostics(t *testing.T) {
	root := repoRoot(t)
	cases := []struct {
		file string
		want string
	}{
		{"unknown_method.walk", "tests/fail/unknown_method.walk:6:14: type error: User has no method is_minor"},
		{"bad_method_arg.walk", "tests/fail/bad_method_arg.walk:10:24: type error: arg 1 to User.rename is string, got int"},
		{"non_struct_method_call.walk", "tests/fail/non_struct_method_call.walk:2:14: type error: method call needs struct receiver, got int"},
		{"bad_method_receiver.walk", "tests/fail/bad_method_receiver.walk:5:1: type error: method User.is_adult receiver param must be User, got int"},
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

func TestV22GenericFixturesBuildAndRun(t *testing.T) {
	requireCC(t)
	root := repoRoot(t)
	cases := []struct {
		file string
		want string
	}{
		{
			file: "generics.walk",
			want: strings.Join([]string{
				"1",
				"Ada",
				"Walker",
				"true",
				"",
			}, "\n"),
		},
		{
			file: "generics_modules.walk",
			want: "8\n",
		},
	}

	for _, tc := range cases {
		t.Run(tc.file, func(t *testing.T) {
			cCode, warnings, err := compileFileToCWithOptions(filepath.Join(root, "tests", "pass", tc.file), false)
			if err != nil {
				t.Fatal(err)
			}
			if len(warnings) != 0 {
				t.Fatalf("unexpected warnings: %#v", warnings)
			}
			if got := runCProgram(t, cCode); got != tc.want {
				t.Fatalf("want %q, got %q\nC:\n%s", tc.want, got, cCode)
			}
		})
	}
}

func TestV22GenericFailFixturesHaveExpectedDiagnostics(t *testing.T) {
	root := repoRoot(t)
	cases := []struct {
		file string
		want string
	}{
		{"bad_generic_arg.walk", "tests/fail/bad_generic_arg.walk:4:16: type error: arg 2 to choose needs T as int, got string"},
		{"generic_function_value.walk", "tests/fail/generic_function_value.walk:4:10: type error: generic function id must be called directly"},
		{"generic_method.walk", "tests/fail/generic_method.walk:4:1: type error: generic methods are not supported yet"},
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

func TestFunctionTypeInferenceBuildsAndRuns(t *testing.T) {
	requireCC(t)
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"func: power_four(n)",
		"    return: ^ n 4",
		"",
		"func: add(a, b)",
		"    return: + a b",
		"",
		"func: half(n float)",
		"    return: / n 2",
		"",
		"func: is_adult(age)",
		"    return: >= age 18",
		"",
		"out: power_four(2)",
		"out: add(2, 3)",
		"out: half(5)",
		"out: is_adult(20)",
		"",
	}, "\n"))

	cCode, warnings, err := compileFileToCWithOptions(sourcePath, false)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}
	if got, want := runCProgram(t, cCode), "16\n5\n2.5\ntrue\n"; got != want {
		t.Fatalf("want %q, got %q\nC:\n%s", want, got, cCode)
	}
	if !strings.Contains(cCode, "WalkInt power_four(WalkInt n)") {
		t.Fatalf("inferred int signature missing from generated C:\n%s", cCode)
	}
	if !strings.Contains(cCode, "WalkFloat half(WalkFloat n)") {
		t.Fatalf("explicit float plus inferred return signature missing from generated C:\n%s", cCode)
	}
}

func TestFunctionTypeInferenceRequiresAmbiguousParamAnnotation(t *testing.T) {
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"func: identity(value)",
		"    return: value",
		"",
		"out: identity(3)",
		"",
	}, "\n"))

	_, err := checkFile(sourcePath)
	if err == nil {
		t.Fatal("expected ambiguous function parameter inference error")
	}
	if got, want := err.Error(), "main.walk:1:1: type error: cannot infer type for parameter value in function identity; add an annotation"; !strings.HasSuffix(got, want) {
		t.Fatalf("want suffix %q, got %q", want, got)
	}
}

func TestV5RuntimeGeneratedCIsInspectableAndArrayStorageIsOwned(t *testing.T) {
	requireCC(t)
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"func: make_numbers() array[int]",
		"    var: nums = [4, 5, 6]",
		"    return: nums",
		"",
		"var: got = make_numbers()",
		"out: got[0]",
		"out: got[2]",
		"",
	}, "\n"))

	cCode, warnings, err := compileFileToCWithOptions(sourcePath, false)
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 0 {
		t.Fatalf("unexpected warnings: %#v", warnings)
	}
	for _, want := range []string{
		"walk runtime: no user pointers",
		"static WALK_UNUSED void *__walk_alloc_array",
		"WalkArrayInt make_numbers(void)",
		"/* source: main.walk:2:5 */",
		"__walk_print_int",
	} {
		if !strings.Contains(cCode, want) {
			t.Fatalf("generated C missing %q:\n%s", want, cCode)
		}
	}
	if got, want := runCProgram(t, cCode), "4\n6\n"; got != want {
		t.Fatalf("want %q, got %q\nC:\n%s", want, got, cCode)
	}
	strictExe := filepath.Join(dir, "strict")
	if err := buildC(cCode, filepath.Join(dir, "strict.c"), strictExe, nativeBuildOptions{cFlags: []string{"-Wall", "-Werror"}}); err != nil {
		t.Fatalf("strict native build failed: %v\nC:\n%s", err, cCode)
	}
	if output, err := exec.Command(strictExe).CombinedOutput(); err != nil || string(output) != "4\n6\n" {
		t.Fatalf("strict program failed: %v\n%s\nC:\n%s", err, string(output), cCode)
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
