package checker

import (
	"strings"
	"testing"

	"walklang/internal/parser"
)

func TestV1ShadowingProducesWarning(t *testing.T) {
	program, err := parser.ParseSource(strings.Join([]string{
		"var: x = 1",
		"if: true",
		"    var: x = 2",
		"    out: x",
	}, "\n"), "main.walk")
	if err != nil {
		t.Fatal(err)
	}

	warnings, err := CheckWithOptions(program, Options{})
	if err != nil {
		t.Fatal(err)
	}
	if len(warnings) != 1 {
		t.Fatalf("want 1 warning, got %d: %#v", len(warnings), warnings)
	}
	if got, want := warnings[0].String(), "main.walk:3:5: warning: x shadows outer name"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}
